#include "HttpServer.hpp"
#include "DirectoryIndexer.hpp"

#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

void HttpServer::handleClientData(int clientSocket) {
    char buffer[4096];
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        if (bytesRead == 0) {
            log.info() << "Client disconnected" << std::endl;
        }

        close(clientSocket);
        _clientSockets.erase(clientSocket);
        return;
    }

    buffer[bytesRead] = '\0';
    log.info() << "Received data from client:\n" << buffer << std::endl;

    // Update client activity time
    updateClientActivity(clientSocket);

    // Parse HTTP request
    std::string requestStr(buffer);
    HttpRequest httpRequest = parseHttpRequest(requestStr);

    std::string method = httpRequest.method;
    std::string path = httpRequest.path;
    std::string version = httpRequest.version;

    log.info() << "HTTP Request: " << method << " " << path << " " << version << std::endl;

    // Check for Connection header
    bool closeConnection = shouldCloseConnection(httpRequest);
    if (closeConnection) {
        log.info() << "Client requested Connection: close" << std::endl;
    } else {
        log.info() << "Client requested Connection: keep-alive" << std::endl;
    }

    // Normalize the URI
    path = normalizeUri(path);
    httpRequest.path = path;

    // Extract query string if present
    std::string query = "";
    size_t queryPos = path.find('?');
    std::string cleanPath = path;

    if (queryPos != std::string::npos) {
        query = path.substr(queryPos + 1);
        cleanPath = path.substr(0, queryPos);
    }

    // Check if this is a CGI script
    if (isCgiScript(cleanPath)) {
        log.info() << "Detected CGI script: " << cleanPath << std::endl;

        // Extract request body for POST requests
        std::string body = "";
        if (method == "POST") {
            body = httpRequest.body;
        }

        // Execute the CGI script
        executeCgi(clientSocket, cleanPath, method, query, body, closeConnection);
        return;
    }

    // Handle different HTTP methods
    if (method == "GET" || method == "HEAD") {
        // Handle GET/HEAD request
        handleGetRequest(clientSocket, httpRequest);
    } else if (method == "DELETE") {
        handleDeleteRequest(clientSocket, path, closeConnection);
    } else if (method == "POST") {
        handlePostRequest(clientSocket, requestStr, path, closeConnection);
    } else {
        // Method not allowed
        std::string connectionHeader = closeConnection ? "close" : "keep-alive";
        std::string methodNotAllowedResponse = "HTTP/1.1 405 Method Not Allowed\r\n"
                                             "Content-Type: text/html\r\n"
                                             "Content-Length: 176\r\n"
                                             "Allow: GET, HEAD, POST, DELETE\r\n"
                                             "Connection: " + connectionHeader + "\r\n"
                                             "\r\n"
                                             "<html><head><title>405 Method Not Allowed</title></head>"
                                             "<body><h1>405 Method Not Allowed</h1>"
                                             "<p>The requested method is not allowed for the URL " + path + ".</p>"
                                             "</body></html>";

        queueWrite(clientSocket, methodNotAllowedResponse);
    }

    // Close connection if requested by client
    if (closeConnection) {
        log.info() << "Closing connection as requested by client" << std::endl;
        close(clientSocket);
        _clientSockets.erase(clientSocket);
    }
}
