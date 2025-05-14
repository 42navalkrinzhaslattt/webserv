#include "HttpServer.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <errno.h>

// Improved version of setupServerSocket that handles more connections
bool HttpServer::setupServerSocket() {
    bool atLeastOneSocketSetup = false;

    for (std::vector<ServerConfig>::iterator it = _serverConfigs.begin(); it != _serverConfigs.end(); ++it) {
        // Create socket
        it->socket = socket(AF_INET, SOCK_STREAM, 0);
        if (it->socket < 0) {
            log.error() << "Failed to create socket for " << it->address << ":" << it->port << ": " << strerror(errno) << std::endl;
            continue;
        }

        // Set socket options for reuse
        if (!setReuseAddr(it->socket)) {
            log.error() << "Failed to set SO_REUSEADDR for " << it->address << ":" << it->port << ": " << strerror(errno) << std::endl;
            close(it->socket);
            it->socket = -1;
            continue;
        }

        // Set TCP_NODELAY to disable Nagle's algorithm
        int flag = 1;
        if (setsockopt(it->socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) < 0) {
            log.error() << "Failed to set TCP_NODELAY for " << it->address << ":" << it->port << ": " << strerror(errno) << std::endl;
            // Not critical, continue anyway
        }

        // Set non-blocking mode
        if (!setNonBlocking(it->socket)) {
            log.error() << "Failed to set non-blocking mode for " << it->address << ":" << it->port << ": " << strerror(errno) << std::endl;
            close(it->socket);
            it->socket = -1;
            continue;
        }

        // Increase socket buffer sizes
        int recvBufSize = 262144; // 256KB
        int sendBufSize = 262144; // 256KB
        
        if (setsockopt(it->socket, SOL_SOCKET, SO_RCVBUF, &recvBufSize, sizeof(recvBufSize)) < 0) {
            log.error() << "Failed to set receive buffer size for " << it->address << ":" << it->port << ": " << strerror(errno) << std::endl;
            // Not critical, continue anyway
        }
        
        if (setsockopt(it->socket, SOL_SOCKET, SO_SNDBUF, &sendBufSize, sizeof(sendBufSize)) < 0) {
            log.error() << "Failed to set send buffer size for " << it->address << ":" << it->port << ": " << strerror(errno) << std::endl;
            // Not critical, continue anyway
        }

        // Bind socket
        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(it->address.c_str());
        address.sin_port = htons(it->port);

        if (bind(it->socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
            log.error() << "Failed to bind socket for " << it->address << ":" << it->port << ": " << strerror(errno) << std::endl;
            close(it->socket);
            it->socket = -1;
            continue;
        }

        // Listen for connections with a larger backlog for Siege testing
        if (listen(it->socket, SOMAXCONN) < 0) {
            log.error() << "Failed to listen on socket for " << it->address << ":" << it->port << ": " << strerror(errno) << std::endl;
            close(it->socket);
            it->socket = -1;
            continue;
        }

        log.info() << "Server listening on " << it->address << ":" << it->port << std::endl;
        atLeastOneSocketSetup = true;
    }

    return atLeastOneSocketSetup;
}

// Improved version of acceptNewConnections that handles more connections
void HttpServer::acceptNewConnections() {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    // Check for new connections on all server sockets
    for (std::vector<ServerConfig>::iterator it = _serverConfigs.begin(); it != _serverConfigs.end(); ++it) {
        if (it->socket <= 0) {
            continue;
        }

        // Accept as many connections as possible
        while (true) {
            // Try to accept a new connection
            int clientSocket = accept(it->socket, (struct sockaddr *)&clientAddr, &clientAddrLen);
            if (clientSocket < 0) {
                // Check if there are no more connections to accept
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                
                log.error() << "Failed to accept connection: " << strerror(errno) << std::endl;
                break;
            }

            // Set non-blocking mode for client socket
            if (!setNonBlocking(clientSocket)) {
                log.error() << "Failed to set non-blocking mode for client socket: " << strerror(errno) << std::endl;
                close(clientSocket);
                continue;
            }

            // Set TCP_NODELAY to disable Nagle's algorithm
            int flag = 1;
            if (setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) < 0) {
                log.error() << "Failed to set TCP_NODELAY for client socket: " << strerror(errno) << std::endl;
                // Not critical, continue anyway
            }

            // Increase socket buffer sizes
            int recvBufSize = 262144; // 256KB
            int sendBufSize = 262144; // 256KB
            
            if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVBUF, &recvBufSize, sizeof(recvBufSize)) < 0) {
                log.error() << "Failed to set receive buffer size for client socket: " << strerror(errno) << std::endl;
                // Not critical, continue anyway
            }
            
            if (setsockopt(clientSocket, SOL_SOCKET, SO_SNDBUF, &sendBufSize, sizeof(sendBufSize)) < 0) {
                log.error() << "Failed to set send buffer size for client socket: " << strerror(errno) << std::endl;
                // Not critical, continue anyway
            }

            // Add the client socket to the set
            _clientSockets.insert(clientSocket);

            // Initialize the client's last activity time
            updateClientActivity(clientSocket);

            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
            log.info() << "New connection from " << clientIP << ":" << ntohs(clientAddr.sin_port) << " (socket " << clientSocket << ")" << std::endl;
        }
    }
}
