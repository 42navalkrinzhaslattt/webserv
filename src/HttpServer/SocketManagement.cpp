#include "HttpServer.hpp"

#include <unistd.h>

void HttpServer::queueWrite(int clientSocket, const std::string &data) {
    ssize_t bytesSent = send(clientSocket, data.c_str(), data.length(), 0);
    if (bytesSent <= 0) {
        log.error() << "Failed to send data to client" << std::endl;
        close(clientSocket);
        _clientSockets.erase(clientSocket);
    }
}

void HttpServer::closeAllSockets() {
    // Close all server sockets
    for (std::vector<ServerConfig>::iterator it = _serverConfigs.begin(); it != _serverConfigs.end(); ++it) {
        if (it->socket > 0) {
            close(it->socket);
            it->socket = -1;
        }
    }

    // Close all client sockets
    for (std::set<int>::iterator it = _clientSockets.begin(); it != _clientSockets.end(); ++it) {
        close(*it);
    }
    _clientSockets.clear();

    log.info() << "All sockets closed" << std::endl;
}
