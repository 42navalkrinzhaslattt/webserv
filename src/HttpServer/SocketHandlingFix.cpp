#include "HttpServer.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>

// Improved version of queueWrite that handles large data more efficiently
void HttpServer::queueWrite(int clientSocket, const std::string &data) {
    // Check if the client socket is still valid
    if (_clientSockets.find(clientSocket) == _clientSockets.end()) {
        log.error() << "Attempted to queue write to invalid socket: " << clientSocket << std::endl;
        return;
    }

    // Try to send the data immediately if possible
    if (_pendingWrites.find(clientSocket) == _pendingWrites.end() || _pendingWrites[clientSocket].empty()) {
        ssize_t bytesSent = send(clientSocket, data.c_str(), data.length(), MSG_NOSIGNAL);

        if (bytesSent <= 0) {
            // Error or connection closed, queue the data for later or close the socket
            _pendingWrites[clientSocket] = data;
            log.debug() << "Socket would block or error, queued " << data.length() << " bytes for client socket " << clientSocket << std::endl;

            // If it's a serious error (not just would block), we'll handle it in the poll loop
            if (bytesSent < 0) {
                log.error() << "Failed to send data to client" << std::endl;
            }
        } else if (static_cast<size_t>(bytesSent) < data.length()) {
            // Not all data was sent, queue the remaining data
            _pendingWrites[clientSocket] = data.substr(bytesSent);
            log.debug() << "Sent " << bytesSent << " bytes, queued " << _pendingWrites[clientSocket].length() << " bytes for client socket " << clientSocket << std::endl;
        } else {
            // All data was sent
            log.debug() << "Sent all " << bytesSent << " bytes to client socket " << clientSocket << std::endl;
        }
    } else {
        // There's already pending data, append to it
        _pendingWrites[clientSocket] += data;
        log.debug() << "Appended " << data.length() << " bytes to pending data for client socket " << clientSocket << std::endl;
    }
}

// Improved version of the event monitoring loop
void HttpServer::run() {
    log.info() << "Starting HTTP server" << std::endl;

    if (!setupServerSocket()) {
        log.error() << "Failed to set up server sockets" << std::endl;
        return;
    }

    _running = true;

    // Main server loop
    while (_running) {
        // Set up poll structures
        std::vector<struct pollfd> fds;
        fds.reserve(_serverConfigs.size() + _clientSockets.size());

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
            clientPollFd.events = POLLIN; // Always monitor for read events

            // Also monitor for write events if there's pending data to write
            if (_pendingWrites.find(*it) != _pendingWrites.end() && !_pendingWrites[*it].empty()) {
                clientPollFd.events |= POLLOUT;
            }

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
            // Check for errors
            if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                // Error on socket
                int errorSocket = fds[i].fd;

                // Check if this is a server socket
                bool isServerSocket = false;
                for (std::vector<ServerConfig>::iterator it = _serverConfigs.begin(); it != _serverConfigs.end(); ++it) {
                    if (errorSocket == it->socket) {
                        isServerSocket = true;
                        break;
                    }
                }

                if (!isServerSocket) {
                    // Client socket error, close it
                    log.error() << "Error on client socket " << errorSocket << std::endl;
                    close(errorSocket);
                    _clientSockets.erase(errorSocket);
                    _pendingWrites.erase(errorSocket);
                } else {
                    // Server socket error, this is more serious
                    log.error() << "Error on server socket " << errorSocket << std::endl;
                }

                continue;
            }

            // Check for read events
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

            // Check for write events
            if (fds[i].revents & POLLOUT) {
                int clientSocket = fds[i].fd;

                // Check if there's pending data to write
                if (_pendingWrites.find(clientSocket) != _pendingWrites.end() && !_pendingWrites[clientSocket].empty()) {
                    // Send the pending data
                    std::string &data = _pendingWrites[clientSocket];
                    ssize_t bytesSent = send(clientSocket, data.c_str(), data.length(), MSG_NOSIGNAL);

                    if (bytesSent <= 0) {
                        // Error or connection closed, handle appropriately
                        if (bytesSent == 0) {
                            log.info() << "Client closed connection during write" << std::endl;
                        } else {
                            log.error() << "Failed to send pending data to client" << std::endl;
                        }
                        // Close the socket and clean up
                        close(clientSocket);
                        _clientSockets.erase(clientSocket);
                        _pendingWrites.erase(clientSocket);
                        _clientLastActivity.erase(clientSocket);
                    } else if (static_cast<size_t>(bytesSent) < data.length()) {
                        // Not all data was sent, keep the remaining data for the next write event
                        data = data.substr(bytesSent);
                        log.debug() << "Sent " << bytesSent << " bytes, " << data.length() << " bytes remaining for client socket " << clientSocket << std::endl;
                    } else {
                        // All data was sent, clear the pending data
                        log.debug() << "Sent all " << bytesSent << " bytes to client socket " << clientSocket << std::endl;
                        _pendingWrites.erase(clientSocket);
                    }
                }
            }
        }
    }

    // Clean up
    closeAllSockets();
}
