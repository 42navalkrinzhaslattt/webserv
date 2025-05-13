#include "HttpServer.hpp"

#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

bool HttpServer::handleRedirect(int clientSocket, const HttpRequest &request, const LocationCtx &location) {
    // Check if the client requested to close the connection
    bool closeConnection = shouldCloseConnection(request);
    log.debug() << "Checking for redirect in location: " << location.first << std::endl;

    // Check if the location has a return directive
    if (directiveExists(location.second, "return")) {
        Arguments returnArgs = getFirstDirective(location.second, "return");

        // Check if we have enough arguments (status code and URL)
        if (returnArgs.size() >= 3) {
            int statusCode = atoi(returnArgs[1].c_str());
            std::string redirectUrl = returnArgs[2];

            log.info() << "Redirecting to " << redirectUrl << " with status code " << statusCode << std::endl;

            // Build redirect response
            std::string connectionHeader = closeConnection ? "close" : "keep-alive";
            std::string redirectResponse = "HTTP/1.1 " + returnArgs[1] + " " + getStatusText(statusCode) + "\r\n"
                                         "Location: " + redirectUrl + "\r\n"
                                         "Content-Length: 0\r\n"
                                         "Connection: " + connectionHeader + "\r\n"
                                         "\r\n";

            ssize_t bytesSent = send(clientSocket, redirectResponse.c_str(), redirectResponse.length(), 0);
            if (bytesSent <= 0) {
                log.error() << "Failed to send redirect response to client" << std::endl;
                close(clientSocket);
                _clientSockets.erase(clientSocket);
                return false;
            }
            return true;
        }
    }

    return false;
}
