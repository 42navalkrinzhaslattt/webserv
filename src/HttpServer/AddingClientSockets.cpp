#include "HttpServer.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

void HttpServer::acceptNewConnections() {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    // Check for new connections on all server sockets
    for (std::vector<ServerConfig>::iterator it = _serverConfigs.begin(); it != _serverConfigs.end(); ++it) {
        if (it->socket <= 0) {
            continue;
        }

        // Try to accept a new connection
        int clientSocket = accept(it->socket, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (clientSocket < 0) {
            continue;
        }

        // Set non-blocking mode for client socket
        int flags = fcntl(clientSocket, F_GETFL, 0);
        if (flags < 0 || fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK) < 0) {
            log.error() << "Failed to set non-blocking mode for client socket" << std::endl;
            close(clientSocket);
            continue;
        }

        // Add client socket to set
        _clientSockets.insert(clientSocket);

        // Update client activity time
        updateClientActivity(clientSocket);

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
        log.info() << "New connection from " << clientIP << ":" << ntohs(clientAddr.sin_port) << " on server " << it->address << ":" << it->port << std::endl;
    }
}
