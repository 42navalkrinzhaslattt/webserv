#include "HttpServer.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <time.h>

void HttpServer::handlePostRequest(int clientSocket, const std::string &request, const std::string &path, bool closeConnection) {
    log.info() << "Handling POST request for path: " << path << std::endl;

    // Parse the HTTP request
    HttpRequest httpRequest = parseHttpRequest(request);

    // Get the location context for this request
    const LocationCtx &location = requestToLocation(clientSocket, httpRequest);

    // Check if the request body exceeds the maximum size
    size_t contentLength = 0;
    if (httpRequest.headers.find("Content-Length") != httpRequest.headers.end()) {
        contentLength = atoi(httpRequest.headers["Content-Length"].c_str());

        // Check if the content length exceeds the maximum allowed size
        if (!checkRequestBodySize(clientSocket, httpRequest, contentLength)) {
            // checkRequestBodySize will send the appropriate error response
            return;
        }
    }

    // Check if this is a file upload request
    if (path == "/upload" || path == "/upload/" || directiveExists(location.second, "upload_dir")) {
        // Check if the request is multipart/form-data
        if (request.find("Content-Type: multipart/form-data") != std::string::npos) {
            std::string boundary;
            std::map<std::string, std::string> formData;

            parseMultipartFormData(request, boundary, formData);

            // Check if we have a file field
            if (formData.find("file") != formData.end()) {
                // Extract filename from Content-Disposition header
                size_t contentDispositionPos = request.find("Content-Disposition: form-data; name=\"file\"");
                if (contentDispositionPos != std::string::npos) {
                    size_t contentDispositionEnd = request.find("\r\n", contentDispositionPos);
                    if (contentDispositionEnd != std::string::npos) {
                        std::string contentDisposition = request.substr(contentDispositionPos, contentDispositionEnd - contentDispositionPos);
                        std::string filename = extractFilename(contentDisposition);

                        if (!filename.empty()) {
                            // Create uploads directory if it doesn't exist
                            std::string uploadDir = "html/default/uploads";
                            struct stat dirStat;
                            if (stat(uploadDir.c_str(), &dirStat) != 0 || !S_ISDIR(dirStat.st_mode)) {
                                mkdir(uploadDir.c_str(), 0755);
                            }

                            // Save the file
                            std::string filePath = uploadDir + "/" + filename;
                            std::ofstream outFile(filePath.c_str(), std::ios::binary);

                            if (outFile.is_open()) {
                                outFile.write(formData["file"].c_str(), formData["file"].length());
                                outFile.close();

                                log.info() << "File uploaded successfully: " << filePath << std::endl;

                                // Create success response content
                                std::string content = "<html><head><title>File Uploaded</title></head>"
                                                    "<body><h1>File Uploaded</h1>"
                                                    "<p>The file " + filename + " was uploaded successfully.</p>"
                                                    "<p><a href=\"/\">Back to home</a></p>"
                                                    "</body></html>";

                                // Use the standard sendString function
                                log.debug() << "Sending response using sendString" << std::endl;
                                sendString(clientSocket, content, 200, "text/html", false, true);

                                // Don't close the connection here, let the standard mechanism handle it
                                return;
                            } else {
                                // Error saving file
                                log.error() << "Failed to save uploaded file: " << filePath << std::endl;

                                std::string connectionHeader = closeConnection ? "close" : "keep-alive";
                                std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\n"
                                                         "Content-Type: text/html\r\n"
                                                         "Content-Length: 144\r\n"
                                                         "Connection: " + connectionHeader + "\r\n"
                                                         "\r\n"
                                                         "<html><head><title>500 Internal Server Error</title></head>"
                                                         "<body><h1>500 Internal Server Error</h1>"
                                                         "<p>An error occurred while saving the uploaded file.</p>"
                                                         "</body></html>";

                                queueWrite(clientSocket, errorResponse);
                            }
                        } else {
                            // No filename provided
                            log.error() << "No filename provided in upload request" << std::endl;

                            std::string connectionHeader = closeConnection ? "close" : "keep-alive";
                            std::string errorResponse = "HTTP/1.1 400 Bad Request\r\n"
                                                     "Content-Type: text/html\r\n"
                                                     "Content-Length: 136\r\n"
                                                     "Connection: " + connectionHeader + "\r\n"
                                                     "\r\n"
                                                     "<html><head><title>400 Bad Request</title></head>"
                                                     "<body><h1>400 Bad Request</h1>"
                                                     "<p>No filename provided in upload request.</p>"
                                                     "</body></html>";

                            queueWrite(clientSocket, errorResponse);
                        }
                    }
                }
            } else {
                // No file field found
                log.error() << "No file field found in upload request" << std::endl;

                std::string connectionHeader = closeConnection ? "close" : "keep-alive";
                std::string errorResponse = "HTTP/1.1 400 Bad Request\r\n"
                                         "Content-Type: text/html\r\n"
                                         "Content-Length: 133\r\n"
                                         "Connection: " + connectionHeader + "\r\n"
                                         "\r\n"
                                         "<html><head><title>400 Bad Request</title></head>"
                                         "<body><h1>400 Bad Request</h1>"
                                         "<p>No file field found in upload request.</p>"
                                         "</body></html>";

                queueWrite(clientSocket, errorResponse);
            }
        } else {
            // Not a multipart/form-data request
            log.error() << "Upload request is not multipart/form-data" << std::endl;

            std::string connectionHeader = closeConnection ? "close" : "keep-alive";
            std::string errorResponse = "HTTP/1.1 400 Bad Request\r\n"
                                     "Content-Type: text/html\r\n"
                                     "Content-Length: 147\r\n"
                                     "Connection: " + connectionHeader + "\r\n"
                                     "\r\n"
                                     "<html><head><title>400 Bad Request</title></head>"
                                     "<body><h1>400 Bad Request</h1>"
                                     "<p>Upload requests must use multipart/form-data encoding.</p>"
                                     "</body></html>";

            queueWrite(clientSocket, errorResponse);
        }
    } else {
        // Not an upload path
        log.error() << "POST request to non-upload path: " << path << std::endl;

        std::string connectionHeader = closeConnection ? "close" : "keep-alive";
        std::string errorResponse = "HTTP/1.1 405 Method Not Allowed\r\n"
                                 "Content-Type: text/html\r\n"
                                 "Content-Length: 152\r\n"
                                 "Connection: " + connectionHeader + "\r\n"
                                 "\r\n"
                                 "<html><head><title>405 Method Not Allowed</title></head>"
                                 "<body><h1>405 Method Not Allowed</h1>"
                                 "<p>POST requests are only allowed at /upload.</p>"
                                 "</body></html>";

        queueWrite(clientSocket, errorResponse);
    }

    // Close connection if requested
    if (closeConnection) {
        close(clientSocket);
        _clientSockets.erase(clientSocket);
    }
}
