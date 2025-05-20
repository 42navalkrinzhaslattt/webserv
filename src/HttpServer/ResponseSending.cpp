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
    queueWrite(clientSocket, responseStr);
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
    queueWrite(clientSocket, headersStr);

    // Send file content if not HEAD request
    if (!headOnly) {
        // Read the entire file into a string and queue it for writing
        std::string fileContent;
        fileContent.reserve(fileSize);

        std::vector<char> buffer(4096);
        while (file.read(&buffer[0], buffer.size())) {
            fileContent.append(&buffer[0], file.gcount());
        }
        if (file.gcount() > 0) {
            fileContent.append(&buffer[0], file.gcount());
        }

        queueWrite(clientSocket, fileContent);
    }

    return true;
}

void HttpServer::sendError(int clientSocket, int statusCode, const LocationCtx *location, bool closeConnection) {
    log.debug() << "Sending error response for status code: " << statusCode << std::endl;

    // Check if custom error page is defined
    std::string errorPagePath;
    std::ostringstream errorCodeStr;
    errorCodeStr << statusCode;

    log.debug() << "Looking for error_page directive for status code: " << statusCode << std::endl;

    // First check in the provided location
    if (location != NULL) {
        ArgResults errorPages = getAllDirectives(location->second, "error_page");
        log.debug() << "Found " << errorPages.size() << " error_page directives in location" << std::endl;

        for (ArgResults::const_iterator it = errorPages.begin(); it != errorPages.end(); ++it) {
            log.debug() << "Checking error_page directive with " << it->size() << " arguments" << std::endl;

            for (size_t i = 0; i < it->size(); ++i) {
                log.debug() << "  Arg[" << i << "] = '" << (*it)[i] << "'" << std::endl;
            }

            for (size_t i = 0; i < it->size() - 1; ++i) {
                if ((*it)[i] == errorCodeStr.str()) {
                    errorPagePath = (*it)[it->size() - 1];
                    log.debug() << "Found error page path for status code " << statusCode << ": " << errorPagePath << std::endl;
                    break;
                }
            }
            if (!errorPagePath.empty()) {
                break;
            }
        }
    }

    // If not found in the location, check in the default location
    if (errorPagePath.empty()) {
        ArgResults errorPages = getAllDirectives(_defaultLocation.second, "error_page");
        log.debug() << "Found " << errorPages.size() << " error_page directives in default location" << std::endl;

        for (ArgResults::const_iterator it = errorPages.begin(); it != errorPages.end(); ++it) {
            log.debug() << "Checking error_page directive with " << it->size() << " arguments" << std::endl;

            for (size_t i = 0; i < it->size(); ++i) {
                log.debug() << "  Arg[" << i << "] = '" << (*it)[i] << "'" << std::endl;
            }

            for (size_t i = 0; i < it->size() - 1; ++i) {
                if ((*it)[i] == errorCodeStr.str()) {
                    errorPagePath = (*it)[it->size() - 1];
                    log.debug() << "Found error page path for status code " << statusCode << ": " << errorPagePath << " in default location" << std::endl;
                    break;
                }
            }
            if (!errorPagePath.empty()) {
                break;
            }
        }
    }

    if (errorPagePath.empty()) {
        log.debug() << "No custom error page found for status code " << statusCode << std::endl;
    }

    if (!errorPagePath.empty()) {
        // Serve custom error page
        // Get the root path from the location
        std::string rootPath = "./";
        if (directiveExists(location->second, "root")) {
            rootPath = getFirstDirective(location->second, "root")[1];
        } else if (directiveExists(_defaultLocation.second, "root")) {
            // If not found in the location, check the default location
            rootPath = getFirstDirective(_defaultLocation.second, "root")[1];
        }

        // Construct the full path to the error page
        std::string diskPath;

        // Handle paths that start with /
        if (!errorPagePath.empty() && errorPagePath[0] == '/') {
            diskPath = rootPath + "/" + errorPagePath.substr(1);
        } else {
            diskPath = rootPath + "/" + errorPagePath;
        }

        // Fix any double slashes
        size_t pos = diskPath.find("//");
        while (pos != std::string::npos) {
            diskPath.replace(pos, 2, "/");
            pos = diskPath.find("//");
        }

        log.debug() << "Looking for error page at: " << diskPath << std::endl;

        std::ifstream file(diskPath.c_str());
        if (file.is_open()) {
            file.close();
            log.debug() << "Found custom error page, sending it" << std::endl;
            sendFileContent(clientSocket, diskPath, *location, statusCode, "", false, closeConnection);
            return;
        } else {
            log.debug() << "Custom error page not found at: " << diskPath << std::endl;
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
