#include "HttpServer.hpp"

#include <sstream>
#include <unistd.h>

void HttpServer::sendString(int clientSocket, const std::string &content, int statusCode, const std::string &contentType, bool headOnly, bool closeConnection) {
    std::ostringstream response;
    std::string connectionHeader = closeConnection ? "close" : "keep-alive";

    response << _httpVersionString << " " << statusCode << " " << getStatusText(statusCode) << "\r\n"
             << "Content-Type: " << (contentType.empty() ? "text/plain" : contentType) << "\r\n"
             << "Content-Length: " << content.length() << "\r\n"
             << "Connection: " << connectionHeader << "\r\n"
             << "\r\n";

    if (!headOnly) {
        response << content;
    }

    std::string responseStr = response.str();
    ssize_t bytesSent = send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
    if (bytesSent <= 0) {
        log.error() << "Failed to send response to client" << std::endl;
        close(clientSocket);
        _clientSockets.erase(clientSocket);
        return;
    }
}

bool HttpServer::sendFileContent(int clientSocket, const std::string &filePath, const LocationCtx &location __attribute__((unused)), int statusCode, const std::string &contentType, bool headOnly, bool closeConnection) {
    std::ifstream file(filePath.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        log.error() << "Failed to open file: " << filePath << std::endl;
        return false;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Determine content type if not provided
    std::string actualContentType = contentType;
    if (actualContentType.empty()) {
        actualContentType = getMimeType(filePath);
    }

    // Send headers
    std::ostringstream headers;
    std::string connectionHeader = closeConnection ? "close" : "keep-alive";

    headers << _httpVersionString << " " << statusCode << " " << getStatusText(statusCode) << "\r\n"
            << "Content-Type: " << actualContentType << "\r\n"
            << "Content-Length: " << fileSize << "\r\n"
            << "Connection: " << connectionHeader << "\r\n"
            << "\r\n";

    std::string headersStr = headers.str();
    ssize_t bytesSent = send(clientSocket, headersStr.c_str(), headersStr.length(), 0);
    if (bytesSent <= 0) {
        log.error() << "Failed to send headers to client" << std::endl;
        close(clientSocket);
        _clientSockets.erase(clientSocket);
        return false;
    }

    // Send file content if not HEAD request
    if (!headOnly) {
        std::vector<char> buffer(4096);
        while (file.read(&buffer[0], buffer.size())) {
            ssize_t bytesSent = send(clientSocket, &buffer[0], file.gcount(), 0);
            if (bytesSent <= 0) {
                log.error() << "Failed to send file content to client" << std::endl;
                close(clientSocket);
                _clientSockets.erase(clientSocket);
                return false;
            }
        }
        if (file.gcount() > 0) {
            ssize_t bytesSent = send(clientSocket, &buffer[0], file.gcount(), 0);
            if (bytesSent <= 0) {
                log.error() << "Failed to send file content to client" << std::endl;
                close(clientSocket);
                _clientSockets.erase(clientSocket);
                return false;
            }
        }
    }

    return true;
}

void HttpServer::sendError(int clientSocket, int statusCode, const LocationCtx *location, bool closeConnection) {
    // Check if custom error page is defined
    std::string errorPagePath;
    if (location != NULL) {
        std::ostringstream errorCodeStr;
        errorCodeStr << statusCode;

        ArgResults errorPages = getAllDirectives(location->second, "error_page");
        for (ArgResults::const_iterator it = errorPages.begin(); it != errorPages.end(); ++it) {
            for (size_t i = 0; i < it->size() - 1; ++i) {
                if ((*it)[i] == errorCodeStr.str()) {
                    errorPagePath = (*it)[it->size() - 1];
                    break;
                }
            }
            if (!errorPagePath.empty()) {
                break;
            }
        }
    }

    if (!errorPagePath.empty()) {
        // Serve custom error page
        std::string diskPath = "html/default" + errorPagePath;
        std::ifstream file(diskPath.c_str());
        if (file.is_open()) {
            file.close();
            sendFileContent(clientSocket, diskPath, *location, statusCode, "", false, closeConnection);
            return;
        }
    }

    // Serve default error page
    std::ostringstream content;
    content << "<html>\n"
            << "<head><title>" << statusCode << " " << getStatusText(statusCode) << "</title></head>\n"
            << "<body>\n"
            << "<h1>" << statusCode << " " << getStatusText(statusCode) << "</h1>\n"
            << "<hr>\n"
            << "<p>Webserv</p>\n"
            << "</body>\n"
            << "</html>\n";

    sendString(clientSocket, content.str(), statusCode, "text/html", false, closeConnection);
}

// These methods are now implemented in InitStatusTexts.cpp and InitMimeTypes.cpp
