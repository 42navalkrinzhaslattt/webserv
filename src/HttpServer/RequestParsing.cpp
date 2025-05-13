#include "HttpServer.hpp"
#include <sstream>

HttpRequest HttpServer::parseHttpRequest(const std::string &requestStr) {
    HttpRequest request;

    // Split the request into lines
    std::vector<std::string> lines;
    std::istringstream requestStream(requestStr);
    std::string line;

    while (std::getline(requestStream, line)) {
        // Remove carriage return if present
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }

        // Empty line marks the end of headers
        if (line.empty()) {
            break;
        }

        lines.push_back(line);
    }

    // Parse the request line
    std::string requestLine = lines[0];
    std::istringstream requestLineStream(requestLine);
    std::string method, path, httpVersion;
    requestLineStream >> method >> path >> httpVersion;

    // Set the request method, path, and HTTP version
    request.method = method;
    request.path = path;
    request.version = httpVersion;

    // Parse the headers
    for (size_t i = 1; i < lines.size(); ++i) {
        size_t colonPos = lines[i].find(':');
        if (colonPos != std::string::npos) {
            std::string headerName = lines[i].substr(0, colonPos);
            std::string headerValue = lines[i].substr(colonPos + 1);

            // Trim leading and trailing whitespace from header value
            headerValue.erase(0, headerValue.find_first_not_of(" \t"));
            headerValue.erase(headerValue.find_last_not_of(" \t") + 1);

            // Store the header
            request.headers[headerName] = headerValue;
        }
    }

    // Extract the request body
    size_t bodyStart = requestStr.find("\r\n\r\n");
    if (bodyStart != std::string::npos) {
        request.body = requestStr.substr(bodyStart + 4);
    }

    return request;
}

bool HttpServer::shouldCloseConnection(const HttpRequest &request) {
    // Check if the Connection header is present
    if (request.headers.find("Connection") != request.headers.end()) {
        // Check if the Connection header value is "close"
        if (request.headers.at("Connection") == "close") {
            return true;
        }
    }

    // Default to keep-alive for HTTP/1.1, close for HTTP/1.0
    return (request.version == "HTTP/1.0");
}
