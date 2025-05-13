#include "HttpServer.hpp"
#include "Utils.hpp"
#include "Repr.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <fstream>

std::string HttpServer::getFileName(const std::string &path) {
    log.debug() << "Getting file name from path: " << repr(path) << std::endl;
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos) {
        log.debug() << "No slash found in path, returning the entire path as filename: " << repr(path) << std::endl;
        return path;
    }

    std::string fileName = path.substr(lastSlash + 1);
    log.debug() << "Extracted filename: " << repr(fileName) << std::endl;
    return fileName;
}

std::string HttpServer::extractFilename(const std::string &contentDisposition) {
    // Extract filename from Content-Disposition header
    // Example: Content-Disposition: form-data; name="file"; filename="example.txt"
    size_t filenamePos = contentDisposition.find("filename=\"");
    if (filenamePos == std::string::npos) {
        return "";
    }

    filenamePos += 10; // Length of 'filename="'
    size_t filenameEnd = contentDisposition.find('"', filenamePos);
    if (filenameEnd == std::string::npos) {
        return "";
    }

    return contentDisposition.substr(filenamePos, filenameEnd - filenamePos);
}

void HttpServer::parseMultipartFormData(const std::string &request, std::string &boundary, std::map<std::string, std::string> &formData) {
    // Extract boundary from Content-Type header
    size_t boundaryPos = request.find("boundary=");
    if (boundaryPos == std::string::npos) {
        log.error() << "No boundary found in Content-Type header" << std::endl;
        return;
    }

    boundaryPos += 9; // Length of 'boundary='

    // Handle quoted boundary
    if (request[boundaryPos] == '"') {
        boundaryPos++; // Skip the opening quote
        size_t boundaryEnd = request.find('"', boundaryPos);
        if (boundaryEnd == std::string::npos) {
            log.error() << "Malformed boundary in Content-Type header" << std::endl;
            return;
        }
        boundary = request.substr(boundaryPos, boundaryEnd - boundaryPos);
    } else {
        // Unquoted boundary
        size_t boundaryEnd = request.find_first_of(" \r\n", boundaryPos);
        if (boundaryEnd == std::string::npos) {
            boundaryEnd = request.length();
        }
        boundary = request.substr(boundaryPos, boundaryEnd - boundaryPos);
    }

    log.debug() << "Found boundary: " << boundary << std::endl;

    // Find the start of the body (after the empty line)
    size_t bodyStart = request.find("\r\n\r\n");
    if (bodyStart == std::string::npos) {
        log.error() << "No empty line found to mark start of body" << std::endl;
        return;
    }
    bodyStart += 4; // Length of '\r\n\r\n'

    // Parse each part of the multipart form data
    std::string fullBoundary = "--" + boundary;
    size_t partStart = request.find(fullBoundary, bodyStart);

    while (partStart != std::string::npos) {
        // Check if this is the final boundary
        if (partStart + fullBoundary.length() + 2 <= request.length() &&
            request.substr(partStart + fullBoundary.length(), 2) == "--") {
            // This is the final boundary
            break;
        }

        // Skip the boundary line
        size_t headerStart = request.find("\r\n", partStart);
        if (headerStart == std::string::npos) {
            log.error() << "Malformed multipart data: no CRLF after boundary" << std::endl;
            break;
        }
        headerStart += 2; // Skip \r\n

        // Find the end of headers (empty line)
        size_t headerEnd = request.find("\r\n\r\n", headerStart);
        if (headerEnd == std::string::npos) {
            log.error() << "Malformed multipart data: no empty line after headers" << std::endl;
            break;
        }

        // Extract headers
        std::string headers = request.substr(headerStart, headerEnd - headerStart);

        // Start of content
        size_t contentStart = headerEnd + 4; // Skip \r\n\r\n

        // Find the start of the next boundary
        size_t nextBoundary = request.find(fullBoundary, contentStart);
        if (nextBoundary == std::string::npos) {
            log.error() << "Malformed multipart data: no closing boundary" << std::endl;
            break;
        }

        // Extract content (excluding the \r\n before the boundary)
        size_t contentEnd = nextBoundary - 2; // -2 for \r\n before boundary
        if (contentEnd < contentStart) {
            // Handle case where there's no content
            contentEnd = contentStart;
        }

        std::string content = request.substr(contentStart, contentEnd - contentStart);

        // Parse Content-Disposition header
        size_t contentDispositionPos = headers.find("Content-Disposition:");
        if (contentDispositionPos != std::string::npos) {
            size_t contentDispositionEnd = headers.find("\r\n", contentDispositionPos);
            if (contentDispositionEnd == std::string::npos) {
                contentDispositionEnd = headers.length();
            }

            std::string contentDisposition = headers.substr(contentDispositionPos, contentDispositionEnd - contentDispositionPos);

            // Extract field name
            size_t namePos = contentDisposition.find("name=\"");
            if (namePos != std::string::npos) {
                namePos += 6; // Length of 'name="'
                size_t nameEnd = contentDisposition.find('"', namePos);
                if (nameEnd != std::string::npos) {
                    std::string name = contentDisposition.substr(namePos, nameEnd - namePos);
                    formData[name] = content;

                    log.debug() << "Found form field: " << name << " with content length: " << content.length() << std::endl;
                }
            }
        }

        // Move to the next part
        partStart = nextBoundary;
    }
}

bool HttpServer::checkRequestBodySize(int clientSocket, const HttpRequest &request, size_t bodySize) {
    log.debug() << "Checking if request body size " << repr(bodySize) << " is within limits" << std::endl;

    // Get client_max_body_size directive
    const LocationCtx &location = requestToLocation(clientSocket, request);
    std::string maxBodySizeStr = getFirstDirective(location.second, "client_max_body_size")[0];
    size_t maxBodySize = Utils::convertSizeToBytes(maxBodySizeStr);

    log.debug() << "Maximum body size allowed: " << repr(maxBodySize) << " bytes" << std::endl;

    if (bodySize > maxBodySize) {
        log.debug() << "Request body size " << repr(bodySize) << " exceeds maximum allowed size " << repr(maxBodySize)
                    << ", sending 413 Request Entity Too Large" << std::endl;
        sendError(clientSocket, 413, &location);
        return false;
    }

    log.debug() << "Request body size " << repr(bodySize) << " is within limits" << std::endl;
    return true;
}

void HttpServer::handleDelete(int clientSocket, const std::string &path) {
    log.debug() << "Handling DELETE request for path: " << repr(path) << std::endl;

    // Construct the file path
    std::string filePath = "html/default" + path;

    // Check if file exists
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) != 0) {
        // File not found, send 404 response
        std::string notFoundResponse = "HTTP/1.1 404 Not Found\r\n"
                                      "Content-Type: text/html\r\n"
                                      "Content-Length: 162\r\n"
                                      "Connection: close\r\n"
                                      "\r\n"
                                      "<html><head><title>404 Not Found</title></head>"
                                      "<body><h1>404 Not Found</h1>"
                                      "<p>The requested URL " + path + " was not found on this server.</p>"
                                      "</body></html>";

        ssize_t bytesSent = send(clientSocket, notFoundResponse.c_str(), notFoundResponse.length(), 0);
        if (bytesSent <= 0) {
            log.error() << "Failed to send not found response to client" << std::endl;
            close(clientSocket);
            _clientSockets.erase(clientSocket);
            return;
        }
        return;
    }

    // Check if it's a directory
    if (S_ISDIR(fileStat.st_mode)) {
        // Cannot delete directories, send 403 response
        std::string forbiddenResponse = "HTTP/1.1 403 Forbidden\r\n"
                                      "Content-Type: text/html\r\n"
                                      "Content-Length: 155\r\n"
                                      "Connection: close\r\n"
                                      "\r\n"
                                      "<html><head><title>403 Forbidden</title></head>"
                                      "<body><h1>403 Forbidden</h1>"
                                      "<p>Cannot delete directory " + path + ".</p>"
                                      "</body></html>";

        ssize_t bytesSent = send(clientSocket, forbiddenResponse.c_str(), forbiddenResponse.length(), 0);
        if (bytesSent <= 0) {
            log.error() << "Failed to send forbidden response to client" << std::endl;
            close(clientSocket);
            _clientSockets.erase(clientSocket);
            return;
        }
        return;
    }

    // Try to delete the file
    if (unlink(filePath.c_str()) != 0) {
        // Error deleting file, send 500 response
        std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\n"
                                   "Content-Type: text/html\r\n"
                                   "Content-Length: 144\r\n"
                                   "Connection: close\r\n"
                                   "\r\n"
                                   "<html><head><title>500 Internal Server Error</title></head>"
                                   "<body><h1>500 Internal Server Error</h1>"
                                   "<p>An error occurred while deleting the file.</p>"
                                   "</body></html>";

        ssize_t bytesSent = send(clientSocket, errorResponse.c_str(), errorResponse.length(), 0);
        if (bytesSent <= 0) {
            log.error() << "Failed to send error response to client" << std::endl;
            close(clientSocket);
            _clientSockets.erase(clientSocket);
            return;
        }
    } else {
        // File deleted successfully, send 200 response
        std::string successResponse = "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: text/html\r\n"
                                    "Content-Length: 129\r\n"
                                    "Connection: close\r\n"
                                    "\r\n"
                                    "<html><head><title>File Deleted</title></head>"
                                    "<body><h1>File Deleted</h1>"
                                    "<p>The file " + path + " was deleted successfully.</p>"
                                    "</body></html>";

        ssize_t bytesSent = send(clientSocket, successResponse.c_str(), successResponse.length(), 0);
        if (bytesSent <= 0) {
            log.error() << "Failed to send success response to client" << std::endl;
            close(clientSocket);
            _clientSockets.erase(clientSocket);
            return;
        }
    }
}

void HttpServer::handleUpload(int clientSocket, const std::string &request, const std::string &path) {
    log.info() << "Handling POST request for path: " << path << std::endl;

    // Create a dummy HttpRequest to get the location context
    HttpRequest dummyRequest;
    dummyRequest.method = "POST";
    dummyRequest.path = path;

    // Get the location context for this request
    const LocationCtx &location = requestToLocation(clientSocket, dummyRequest);

    // Check if this is a file upload request (location has upload_dir directive)
    if (directiveExists(location.second, "upload_dir")) {
        // Get the upload directory from the configuration
        std::string uploadDirPath = getFirstDirective(location.second, "upload_dir")[1];
        log.debug() << "Upload directory from config: " << uploadDirPath << std::endl;

        // Check client_max_body_size directive
        size_t maxBodySize = 10 * 1024 * 1024; // Default: 10MB
        if (directiveExists(location.second, "client_max_body_size")) {
            std::string maxBodySizeStr = getFirstDirective(location.second, "client_max_body_size")[1];
            log.debug() << "Max body size from config: " << maxBodySizeStr << std::endl;

            // Parse the size (e.g., 1m, 500k, 10g)
            size_t size = 0;
            char unit = maxBodySizeStr[maxBodySizeStr.length() - 1];
            if (isdigit(unit)) {
                // No unit specified, assume bytes
                size = atoi(maxBodySizeStr.c_str());
            } else {
                // Parse the number part
                size = atoi(maxBodySizeStr.substr(0, maxBodySizeStr.length() - 1).c_str());

                // Apply the unit multiplier
                switch (tolower(unit)) {
                    case 'k': size *= 1024; break;            // Kilobytes
                    case 'm': size *= 1024 * 1024; break;    // Megabytes
                    case 'g': size *= 1024 * 1024 * 1024; break; // Gigabytes
                    default: size = 0; // Invalid unit
                }
            }

            if (size > 0) {
                maxBodySize = size;
            }
        }

        // Check if the request body exceeds the maximum size
        size_t contentLength = 0;
        size_t contentLengthPos = request.find("Content-Length: ");
        if (contentLengthPos != std::string::npos) {
            size_t contentLengthEnd = request.find("\r\n", contentLengthPos);
            if (contentLengthEnd != std::string::npos) {
                std::string contentLengthStr = request.substr(contentLengthPos + 16, contentLengthEnd - (contentLengthPos + 16));
                contentLength = atoi(contentLengthStr.c_str());
                log.debug() << "Content length: " << contentLength << ", Max body size: " << maxBodySize << std::endl;

                if (contentLength > maxBodySize) {
                    log.error() << "Request body too large: " << contentLength << " bytes (max: " << maxBodySize << " bytes)" << std::endl;

                    std::string errorResponse = "HTTP/1.1 413 Request Entity Too Large\r\n"
                                             "Content-Type: text/html\r\n"
                                             "Content-Length: 163\r\n"
                                             "Connection: close\r\n"
                                             "\r\n"
                                             "<html><head><title>413 Request Entity Too Large</title></head>"
                                             "<body><h1>413 Request Entity Too Large</h1>"
                                             "<p>The request body exceeds the maximum allowed size.</p>"
                                             "</body></html>";

                    ssize_t bytesSent = send(clientSocket, errorResponse.c_str(), errorResponse.length(), 0);
                    if (bytesSent <= 0) {
                        log.error() << "Failed to send error response to client" << std::endl;
                        close(clientSocket);
                        _clientSockets.erase(clientSocket);
                    }
                    return;
                }
            }
        }

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
                            try {
                                // Create uploads directory if it doesn't exist
                                std::string uploadDir = "html/default" + uploadDirPath;
                                log.debug() << "Full upload directory path: " << uploadDir << std::endl;

                                // Create directory recursively if needed
                                size_t pos = 0;
                                std::string dirPath = "html/default";
                                while ((pos = uploadDirPath.find('/', pos + 1)) != std::string::npos) {
                                    std::string subDir = uploadDirPath.substr(0, pos);
                                    dirPath = "html/default" + subDir;

                                    struct stat dirStat;
                                    if (stat(dirPath.c_str(), &dirStat) != 0) {
                                        log.info() << "Creating directory: " << dirPath << std::endl;
                                        if (mkdir(dirPath.c_str(), 0755) != 0) {
                                            log.error() << "Failed to create directory: " << dirPath << std::endl;
                                            throw std::runtime_error("Failed to create directory");
                                        }
                                    } else if (!S_ISDIR(dirStat.st_mode)) {
                                        log.error() << "Path exists but is not a directory: " << dirPath << std::endl;
                                        throw std::runtime_error("Path exists but is not a directory");
                                    }
                                }

                                // Create the final directory
                                struct stat dirStat;
                                if (stat(uploadDir.c_str(), &dirStat) != 0) {
                                    log.info() << "Creating upload directory: " << uploadDir << std::endl;
                                    if (mkdir(uploadDir.c_str(), 0755) != 0) {
                                        log.error() << "Failed to create upload directory: " << uploadDir << std::endl;
                                        throw std::runtime_error("Failed to create upload directory");
                                    }
                                } else if (!S_ISDIR(dirStat.st_mode)) {
                                    log.error() << "Upload path exists but is not a directory: " << uploadDir << std::endl;
                                    throw std::runtime_error("Upload path exists but is not a directory");
                                }

                                // Sanitize filename to prevent directory traversal attacks
                                std::string sanitizedFilename = filename;
                                size_t slashPos;
                                while ((slashPos = sanitizedFilename.find('/')) != std::string::npos) {
                                    sanitizedFilename.replace(slashPos, 1, "_");
                                }
                                while ((slashPos = sanitizedFilename.find('\\')) != std::string::npos) {
                                    sanitizedFilename.replace(slashPos, 1, "_");
                                }

                                // Save the file
                                std::string filePath = uploadDir + "/" + sanitizedFilename;
                                log.debug() << "Saving file to: " << filePath << std::endl;

                                std::ofstream outFile(filePath.c_str(), std::ios::binary);
                                if (!outFile.is_open()) {
                                    log.error() << "Failed to open file for writing: " << filePath << std::endl;
                                    throw std::runtime_error("Failed to open file for writing");
                                }

                                outFile.write(formData["file"].c_str(), formData["file"].length());
                                if (outFile.fail()) {
                                    outFile.close();
                                    log.error() << "Failed to write to file: " << filePath << std::endl;
                                    throw std::runtime_error("Failed to write to file");
                                }

                                outFile.close();
                                log.info() << "File uploaded successfully: " << filePath << std::endl;

                                // Send success response
                                std::string successResponse = "HTTP/1.1 200 OK\r\n"
                                                            "Content-Type: text/html\r\n"
                                                            "Content-Length: 200\r\n"
                                                            "Connection: close\r\n"
                                                            "\r\n"
                                                            "<html><head><title>File Uploaded</title></head>"
                                                            "<body><h1>File Uploaded</h1>"
                                                            "<p>The file " + sanitizedFilename + " was uploaded successfully.</p>"
                                                            "<p><a href=\"/\">Back to home</a></p>"
                                                            "</body></html>";

                                ssize_t bytesSent = send(clientSocket, successResponse.c_str(), successResponse.length(), 0);
                                if (bytesSent <= 0) {
                                    log.error() << "Failed to send success response to client" << std::endl;
                                    close(clientSocket);
                                    _clientSockets.erase(clientSocket);
                                    return;
                                }
                            } catch (const std::exception &e) {
                                // Error saving file
                                log.error() << "Failed to save uploaded file: " << e.what() << std::endl;

                                std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\n"
                                                         "Content-Type: text/html\r\n"
                                                         "Content-Length: 144\r\n"
                                                         "Connection: close\r\n"
                                                         "\r\n"
                                                         "<html><head><title>500 Internal Server Error</title></head>"
                                                         "<body><h1>500 Internal Server Error</h1>"
                                                         "<p>An error occurred while saving the uploaded file.</p>"
                                                         "</body></html>";

                                ssize_t bytesSent = send(clientSocket, errorResponse.c_str(), errorResponse.length(), 0);
                                if (bytesSent <= 0) {
                                    log.error() << "Failed to send error response to client" << std::endl;
                                    close(clientSocket);
                                    _clientSockets.erase(clientSocket);
                                    return;
                                }
                            }
                        } else {
                            // No filename provided
                            log.error() << "No filename provided in upload request" << std::endl;

                            std::string errorResponse = "HTTP/1.1 400 Bad Request\r\n"
                                                     "Content-Type: text/html\r\n"
                                                     "Content-Length: 136\r\n"
                                                     "Connection: close\r\n"
                                                     "\r\n"
                                                     "<html><head><title>400 Bad Request</title></head>"
                                                     "<body><h1>400 Bad Request</h1>"
                                                     "<p>No filename provided in upload request.</p>"
                                                     "</body></html>";

                            ssize_t bytesSent = send(clientSocket, errorResponse.c_str(), errorResponse.length(), 0);
                            if (bytesSent <= 0) {
                                log.error() << "Failed to send error response to client" << std::endl;
                                close(clientSocket);
                                _clientSockets.erase(clientSocket);
                                return;
                            }
                        }
                    }
                }
            } else {
                // No file field found
                log.error() << "No file field found in upload request" << std::endl;

                std::string errorResponse = "HTTP/1.1 400 Bad Request\r\n"
                                         "Content-Type: text/html\r\n"
                                         "Content-Length: 133\r\n"
                                         "Connection: close\r\n"
                                         "\r\n"
                                         "<html><head><title>400 Bad Request</title></head>"
                                         "<body><h1>400 Bad Request</h1>"
                                         "<p>No file field found in upload request.</p>"
                                         "</body></html>";

                ssize_t bytesSent = send(clientSocket, errorResponse.c_str(), errorResponse.length(), 0);
                if (bytesSent <= 0) {
                    log.error() << "Failed to send error response to client" << std::endl;
                    close(clientSocket);
                    _clientSockets.erase(clientSocket);
                    return;
                }
            }
        } else {
            // Not a multipart/form-data request
            log.error() << "Upload request is not multipart/form-data" << std::endl;

            std::string errorResponse = "HTTP/1.1 400 Bad Request\r\n"
                                     "Content-Type: text/html\r\n"
                                     "Content-Length: 147\r\n"
                                     "Connection: close\r\n"
                                     "\r\n"
                                     "<html><head><title>400 Bad Request</title></head>"
                                     "<body><h1>400 Bad Request</h1>"
                                     "<p>Upload requests must use multipart/form-data encoding.</p>"
                                     "</body></html>";

            ssize_t bytesSent = send(clientSocket, errorResponse.c_str(), errorResponse.length(), 0);
            if (bytesSent <= 0) {
                log.error() << "Failed to send error response to client" << std::endl;
                close(clientSocket);
                _clientSockets.erase(clientSocket);
                return;
            }
        }
    } else {
        // Not an upload path (no upload_dir directive)
        log.error() << "POST request to non-upload path: " << path << std::endl;

        std::string errorResponse = "HTTP/1.1 405 Method Not Allowed\r\n"
                                 "Content-Type: text/html\r\n"
                                 "Content-Length: 152\r\n"
                                 "Connection: close\r\n"
                                 "\r\n"
                                 "<html><head><title>405 Method Not Allowed</title></head>"
                                 "<body><h1>405 Method Not Allowed</h1>"
                                 "<p>POST requests are only allowed at paths with upload_dir directive.</p>"
                                 "</body></html>";

        ssize_t bytesSent = send(clientSocket, errorResponse.c_str(), errorResponse.length(), 0);
        if (bytesSent <= 0) {
            log.error() << "Failed to send error response to client" << std::endl;
            close(clientSocket);
            _clientSockets.erase(clientSocket);
            return;
        }
    }
}









