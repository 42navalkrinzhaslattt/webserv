#include "HttpServer.hpp"
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

// Improved version of handleClientData that handles more requests
void HttpServer::handleClientData(int clientSocket) {
    // Use a larger buffer for better performance
    char buffer[16384]; // 16KB buffer
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        if (bytesRead == 0) {
            log.info() << "Client disconnected (socket " << clientSocket << ")" << std::endl;
        } else {
            log.error() << "Error reading from client (socket " << clientSocket << ")" << std::endl;
        }

        close(clientSocket);
        _clientSockets.erase(clientSocket);
        _pendingWrites.erase(clientSocket);
        _clientLastActivity.erase(clientSocket);
        return;
    }

    buffer[bytesRead] = '\0';
    log.debug() << "Received " << bytesRead << " bytes from client (socket " << clientSocket << ")" << std::endl;

    // Update client activity time
    updateClientActivity(clientSocket);

    // Parse HTTP request
    std::string requestStr(buffer, bytesRead);
    HttpRequest httpRequest = parseHttpRequest(requestStr);

    std::string method = httpRequest.method;
    std::string path = httpRequest.path;
    std::string version = httpRequest.version;

    log.info() << "HTTP Request: " << method << " " << path << " " << version << " (socket " << clientSocket << ")" << std::endl;

    // Normalize the URI
    path = normalizeUri(path);
    httpRequest.path = path;

    // Check for Connection header
    bool closeConnection = shouldCloseConnection(httpRequest);

    // Handle the request based on the method
    try {
        if (method == "GET" || method == "HEAD") {
            handleGetRequest(clientSocket, httpRequest);
        } else if (method == "POST") {
            handlePostRequest(clientSocket, requestStr, path, closeConnection);
        } else if (method == "DELETE") {
            handleDeleteRequest(clientSocket, path, closeConnection);
        } else {
            // Unknown method
            sendError(clientSocket, 501, NULL, closeConnection);
        }
    } catch (const std::exception &e) {
        log.error() << "Exception while handling request: " << e.what() << std::endl;
        sendError(clientSocket, 500, NULL, closeConnection);
    } catch (...) {
        log.error() << "Unknown exception while handling request" << std::endl;
        sendError(clientSocket, 500, NULL, closeConnection);
    }

    // If the client requested to close the connection, close it
    if (closeConnection) {
        log.info() << "Closing connection as requested by client (socket " << clientSocket << ")" << std::endl;

        // Wait for any pending writes to complete
        if (_pendingWrites.find(clientSocket) != _pendingWrites.end() && !_pendingWrites[clientSocket].empty()) {
            log.debug() << "Waiting for pending writes to complete before closing connection" << std::endl;
            // The connection will be closed when all pending writes are complete
        } else {
            // No pending writes, close the connection now
            close(clientSocket);
            _clientSockets.erase(clientSocket);
            _clientLastActivity.erase(clientSocket);
        }
    }
}
