#include "HttpServer.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

void HttpServer::handleDeleteRequest(int clientSocket, const std::string &path, bool closeConnection) {
    log.debug() << "Handling DELETE request for path: " << path << std::endl;

    // Construct the file path
    std::string filePath = "html/default" + path;

    // Check if file exists
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) != 0) {
        // File not found, send 404 response
        std::string connectionHeader = closeConnection ? "close" : "keep-alive";
        std::string notFoundResponse = "HTTP/1.1 404 Not Found\r\n"
                                      "Content-Type: text/html\r\n"
                                      "Content-Length: 162\r\n"
                                      "Connection: " + connectionHeader + "\r\n"
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

        // Close connection if requested
        if (closeConnection) {
            close(clientSocket);
            _clientSockets.erase(clientSocket);
        }
        return;
    }

    // Check if it's a directory
    if (S_ISDIR(fileStat.st_mode)) {
        // Cannot delete directories, send 403 response
        std::string connectionHeader = closeConnection ? "close" : "keep-alive";
        std::string forbiddenResponse = "HTTP/1.1 403 Forbidden\r\n"
                                      "Content-Type: text/html\r\n"
                                      "Content-Length: 155\r\n"
                                      "Connection: " + connectionHeader + "\r\n"
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

        // Close connection if requested
        if (closeConnection) {
            close(clientSocket);
            _clientSockets.erase(clientSocket);
        }
        return;
    }

    // Try to delete the file
    if (unlink(filePath.c_str()) != 0) {
        // Error deleting file, send 500 response
        std::string connectionHeader = closeConnection ? "close" : "keep-alive";
        std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\n"
                                   "Content-Type: text/html\r\n"
                                   "Content-Length: 144\r\n"
                                   "Connection: " + connectionHeader + "\r\n"
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
        std::string connectionHeader = closeConnection ? "close" : "keep-alive";
        std::string successResponse = "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: text/html\r\n"
                                    "Content-Length: 129\r\n"
                                    "Connection: " + connectionHeader + "\r\n"
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

    // Close connection if requested
    if (closeConnection) {
        close(clientSocket);
        _clientSockets.erase(clientSocket);
    }
}
