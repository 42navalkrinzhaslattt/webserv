#include "HttpServer.hpp"

#include <unistd.h>

void HttpServer::queueWrite(int clientSocket, const std::string &data) {
    // Add the data to the pending writes map
    if (_pendingWrites.find(clientSocket) != _pendingWrites.end()) {
        // Append to existing pending data
        _pendingWrites[clientSocket] += data;
    } else {
        // Create new pending data
        _pendingWrites[clientSocket] = data;
    }
    log.debug() << "Queued " << data.length() << " bytes for client socket " << clientSocket << std::endl;
}

bool HttpServer::canWriteToSocket(int clientSocket) {
    return _clientSockets.find(clientSocket) != _clientSockets.end();
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
