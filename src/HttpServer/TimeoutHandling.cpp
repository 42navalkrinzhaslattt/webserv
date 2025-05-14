#include "HttpServer.hpp"

#include <sys/time.h>
#include <unistd.h>

void HttpServer::initTimeouts() {
    log.debug() << "Initializing timeouts" << std::endl;

    // Set default timeout values - increased for Siege testing
    _connectionTimeout = 120; // 120 seconds
    _keepAliveTimeout = 30;   // 30 seconds
    _readTimeout = 60;        // 60 seconds
    _writeTimeout = 60;       // 60 seconds

    // Initialize the current time
    gettimeofday(&_currentTime, NULL);

    log.debug() << "Timeouts initialized: connection=" << _connectionTimeout
                << "s, keep-alive=" << _keepAliveTimeout
                << "s, read=" << _readTimeout
                << "s, write=" << _writeTimeout << "s" << std::endl;
}

void HttpServer::updateCurrentTime() {
    gettimeofday(&_currentTime, NULL);
}

time_t HttpServer::getCurrentTime() const {
    return _currentTime.tv_sec;
}

void HttpServer::checkTimeouts() {
    // Update the current time
    updateCurrentTime();

    // Check for client timeouts
    std::vector<int> timedOutClients;

    for (std::map<int, time_t>::iterator it = _clientLastActivity.begin(); it != _clientLastActivity.end(); ++it) {
        int clientSocket = it->first;
        time_t lastActivity = it->second;

        // Check if the client has timed out
        if (_currentTime.tv_sec - lastActivity > _connectionTimeout) {
            log.debug() << "Client " << clientSocket << " timed out after " << (_currentTime.tv_sec - lastActivity) << " seconds" << std::endl;
            timedOutClients.push_back(clientSocket);
        }
    }

    // Close timed out client connections
    for (std::vector<int>::iterator it = timedOutClients.begin(); it != timedOutClients.end(); ++it) {
        int clientSocket = *it;

        // Send a 408 Request Timeout response
        std::string timeoutResponse = "HTTP/1.1 408 Request Timeout\r\n"
                                     "Content-Type: text/html\r\n"
                                     "Content-Length: 166\r\n"
                                     "Connection: close\r\n"
                                     "\r\n"
                                     "<html><head><title>408 Request Timeout</title></head>"
                                     "<body><h1>408 Request Timeout</h1>"
                                     "<p>The server timed out waiting for the request.</p>"
                                     "</body></html>";

        // Even if send fails, we still need to close the connection for a timeout
        queueWrite(clientSocket, timeoutResponse);

        // Close the connection
        close(clientSocket);

        // Remove the client from the activity map and socket set
        _clientLastActivity.erase(clientSocket);
        _clientSockets.erase(clientSocket);

        log.info() << "Closed timed out client connection: " << clientSocket << std::endl;
    }
}

void HttpServer::updateClientActivity(int clientSocket) {
    // Update the last activity time for the client
    _clientLastActivity[clientSocket] = _currentTime.tv_sec;
}

void HttpServer::setConnectionTimeout(time_t timeout) {
    _connectionTimeout = timeout;
    log.debug() << "Connection timeout set to " << timeout << " seconds" << std::endl;
}

void HttpServer::setKeepAliveTimeout(time_t timeout) {
    _keepAliveTimeout = timeout;
    log.debug() << "Keep-alive timeout set to " << timeout << " seconds" << std::endl;
}

void HttpServer::setReadTimeout(time_t timeout) {
    _readTimeout = timeout;
    log.debug() << "Read timeout set to " << timeout << " seconds" << std::endl;
}

void HttpServer::setWriteTimeout(time_t timeout) {
    _writeTimeout = timeout;
    log.debug() << "Write timeout set to " << timeout << " seconds" << std::endl;
}
