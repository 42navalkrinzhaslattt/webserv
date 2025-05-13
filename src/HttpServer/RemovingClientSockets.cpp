#include "HttpServer.hpp"

#include <unistd.h>

void HttpServer::removeClientSocket(int clientSocket) {
    if (_clientSockets.find(clientSocket) != _clientSockets.end()) {
        close(clientSocket);
        _clientSockets.erase(clientSocket);
        log.debug() << "Removed client socket: " << clientSocket << std::endl;
    }
}
