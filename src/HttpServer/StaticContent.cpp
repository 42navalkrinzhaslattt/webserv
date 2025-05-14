#include "HttpServer.hpp"
#include "DirectoryIndexer.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <sstream>

// This file contains functions for serving static content

bool HttpServer::serveStaticFile(int clientSocket, const std::string &path, const HttpRequest &request) {
    // Check if the client requested to close the connection
    bool closeConnection = shouldCloseConnection(request);
    log.debug() << "Serving static file: " << path << std::endl;

    // Get the location context for this request
    const LocationCtx &location = requestToLocation(clientSocket, request);

    // Determine the disk path for the requested resource
    std::string diskPath = determineDiskPath(request, location);

    // Check if path exists
    struct stat pathStat;
    bool fileExists = false;
    bool isDirectory = false;

    if (stat(diskPath.c_str(), &pathStat) == 0) {
        fileExists = true;
        isDirectory = S_ISDIR(pathStat.st_mode);
    }

    // If it's a directory, check for index files or generate directory listing
    if (fileExists && isDirectory) {
        // Check if path ends with a slash
        std::string cleanPath = path;
        size_t queryPos = cleanPath.find('?');
        if (queryPos != std::string::npos) {
            cleanPath = cleanPath.substr(0, queryPos);
        }

        if (cleanPath.length() > 0 && cleanPath[cleanPath.length() - 1] != '/') {
            // Redirect to add trailing slash
            std::string connectionHeader = closeConnection ? "close" : "keep-alive";
            std::string redirectResponse = "HTTP/1.1 301 Moved Permanently\r\n"
                                         "Location: " + cleanPath + "/\r\n"
                                         "Content-Length: 0\r\n"
                                         "Connection: " + connectionHeader + "\r\n"
                                         "\r\n";

            queueWrite(clientSocket, redirectResponse);
            return true;
        }

        // Check for index files
        if (handleIndexes(clientSocket, diskPath, request, location)) {
            return true;
        }

        // If no index file found, check if autoindex is enabled
        if (directiveExists(location.second, "autoindex")) {
            Arguments autoindexArgs = getFirstDirective(location.second, "autoindex");
            if (autoindexArgs.size() > 1 && autoindexArgs[1] == "on") {
                // Generate directory listing
                DirectoryIndexer indexer(log);
                std::string listing = indexer.indexDirectory(cleanPath, diskPath);

                // Send HTTP response
                sendString(clientSocket, listing, 200, "text/html", request.method == "HEAD", closeConnection);
                return true;
            }
        }

        // Directory listing not allowed, send 403 Forbidden
        sendError(clientSocket, 403, &location, closeConnection);
        return true;
    }

    // Check if file exists and is not a directory
    if (fileExists && !isDirectory) {
        // Serve the file
        return sendFileContent(clientSocket, diskPath, location, 200, "", request.method == "HEAD", closeConnection);
    }

    // File not found, send 404 response
    sendError(clientSocket, 404, &location, closeConnection);
    return true;
}
