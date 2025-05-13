#include "HttpServer.hpp"

#include <poll.h>
#include <cstring>

void HttpServer::run() {
    log.info() << "Starting HTTP server" << std::endl;
    log.debug() << "_locations address in run: " << &_locations << std::endl;
    log.debug() << "_locations size in run: " << _locations.size() << std::endl;

    if (!_locations.empty()) {
        log.debug() << "_locations[0].first: " << _locations[0].first << std::endl;
    }

    if (!setupServerSocket()) {
        log.error() << "Failed to set up server sockets" << std::endl;
        return;
    }

    _running = true;

    // Main server loop
    while (_running) {
        // Set up poll structures
        std::vector<struct pollfd> fds;

        // Add server sockets
        for (std::vector<ServerConfig>::iterator it = _serverConfigs.begin(); it != _serverConfigs.end(); ++it) {
            if (it->socket > 0) {
                struct pollfd serverPollFd;
                serverPollFd.fd = it->socket;
                serverPollFd.events = POLLIN;
                serverPollFd.revents = 0;
                fds.push_back(serverPollFd);
            }
        }

        // Add client sockets
        for (std::set<int>::iterator it = _clientSockets.begin(); it != _clientSockets.end(); ++it) {
            struct pollfd clientPollFd;
            clientPollFd.fd = *it;
            clientPollFd.events = POLLIN;
            clientPollFd.revents = 0;
            fds.push_back(clientPollFd);
        }

        // Check for timeouts
        checkTimeouts();

        // Wait for events with a shorter timeout for better responsiveness
        int pollResult = poll(&fds[0], fds.size(), 100); // 100 milliseconds timeout

        if (pollResult < 0) {
            log.error() << "Poll error occurred" << std::endl;
            break;
        }

        if (pollResult == 0) {
            continue; // Timeout, continue loop
        }

        // Check for events
        for (size_t i = 0; i < fds.size(); ++i) {
            if (fds[i].revents & POLLIN) {
                // Check if this is a server socket
                bool isServerSocket = false;
                for (std::vector<ServerConfig>::iterator it = _serverConfigs.begin(); it != _serverConfigs.end(); ++it) {
                    if (fds[i].fd == it->socket) {
                        // New connection
                        isServerSocket = true;
                        break;
                    }
                }

                if (isServerSocket) {
                    // New connection
                    acceptNewConnections();
                } else {
                    // Client data
                    handleClientData(fds[i].fd);
                }
            }
        }
    }

    log.info() << "HTTP server stopped" << std::endl;
}
