#include "Server.hpp"
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>
#include <ctime>
#include <set>
#include <sys/wait.h>
#include <cctype>

Server::Server(const std::string &confPath) : running(false) {
    // Load configuration from file
    loadConfig(confPath);
}

void Server::loadConfig(const string& confPath) {
    // Default configuration if no file is provided
    if (confPath.empty()) {
        ServerConfig defaultConfig;
        LocationConfig defaultLocation;
        defaultLocation.allowedMethods.push_back("GET");
        defaultLocation.allowedMethods.push_back("POST");
        defaultLocation.allowedMethods.push_back("DELETE");
        defaultLocation.index.push_back("index.html");
        defaultConfig.locations.push_back(defaultLocation);
        serverConfigs.push_back(defaultConfig);
        return;
    }

    // Open and parse the configuration file
    std::ifstream configFile(confPath.c_str());
    if (!configFile.is_open()) {
        std::cerr << "Failed to open configuration file: " << confPath << std::endl;
        // Fall back to default configuration
        loadConfig("");
        return;
    }

    ServerConfig* currentServer = NULL;
    LocationConfig* currentLocation = NULL;
    string line;

    while (std::getline(configFile, line)) {
        // Remove comments and trim whitespace
        size_t commentPos = line.find('#');
        if (commentPos != string::npos) {
            line = line.substr(0, commentPos);
        }
        Utils::ft_trim(line);

        if (line.empty()) {
            continue;
        }

        // Parse server block
        if (line == "server {") {
            serverConfigs.push_back(ServerConfig());
            currentServer = &serverConfigs.back();
            currentLocation = NULL;
        }
        // Parse location block
        else if (line.substr(0, 9) == "location " && line.find("{") != string::npos) {
            if (!currentServer) {
                std::cerr << "Error: location block outside of server block" << std::endl;
                continue;
            }

            string path = line.substr(9, line.find("{") - 10);
            Utils::ft_trim(path);

            currentServer->locations.push_back(LocationConfig());
            currentLocation = &currentServer->locations.back();
            currentLocation->path = path;

            // Inherit root from server if not specified
            currentLocation->root = currentServer->root;
        }
        // End of block
        else if (line == "}") {
            currentLocation = NULL;
        }
        // Parse directives
        else if (line.find(";") != string::npos) {
            string directive = line.substr(0, line.find(" "));
            string value = line.substr(line.find(" ") + 1, line.find(";") - line.find(" ") - 1);
            Utils::ft_trim(value);

            if (currentLocation) {
                // Location block directives
                if (directive == "root") {
                    currentLocation->root = value;
                }
                else if (directive == "index") {
                    currentLocation->index = Utils::ft_split(value);
                }
                else if (directive == "autoindex") {
                    currentLocation->autoindex = (value == "on");
                }
                else if (directive == "return") {
                    currentLocation->redirect = value;
                }
                else if (directive == "limit_except") {
                    currentLocation->allowedMethods = Utils::ft_split(value);
                }
                else if (directive == "client_max_body_size") {
                    currentLocation->clientMaxBodySize = std::strtoul(value.c_str(), NULL, 10);
                }
                else if (directive == "upload_path") {
                    currentLocation->uploadPath = value;
                }
                else if (directive == "cgi_handler") {
                    // Format: cgi_handler .php /usr/bin/php-cgi;
                    size_t spacePos = value.find(' ');
                    if (spacePos != string::npos) {
                        string extension = value.substr(0, spacePos);
                        string handler = value.substr(spacePos + 1);
                        Utils::ft_trim(extension);
                        Utils::ft_trim(handler);
                        currentLocation->cgiHandlers[extension] = handler;
                        std::cout << "Added CGI handler for " << extension << ": " << handler << std::endl;
                    }
                }
            }
            else if (currentServer) {
                // Server block directives
                if (directive == "listen") {
                    currentServer->port = static_cast<int>(std::strtol(value.c_str(), NULL, 10));
                }
                else if (directive == "server_name") {
                    currentServer->serverNames = Utils::ft_split(value);
                }
                else if (directive == "root") {
                    currentServer->root = value;
                }
            }
        }
    }

    // If no server blocks were defined, use default configuration
    if (serverConfigs.empty()) {
        loadConfig("");
    }

    // Ensure each server has at least one location block
    for (size_t i = 0; i < serverConfigs.size(); ++i) {
        if (serverConfigs[i].locations.empty()) {
            LocationConfig defaultLocation;
            defaultLocation.allowedMethods.push_back("GET");
            defaultLocation.allowedMethods.push_back("POST");
            defaultLocation.allowedMethods.push_back("DELETE");
            defaultLocation.index.push_back("index.html");
            defaultLocation.root = serverConfigs[i].root;
            serverConfigs[i].locations.push_back(defaultLocation);
        }
    }

    std::cout << "Loaded " << serverConfigs.size() << " server configurations" << std::endl;
}

Server::~Server() {
    stop();
}

bool Server::setNonBlocking(int fd) {
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return false;
    return ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

// Match request to server block based on host header and port
Server::ServerConfig* Server::matchServerConfig(const HttpRequest& /* request */, const string& host, int port) {
    std::cout << "Matching server for host: '" << host << "' on port: " << port << std::endl;

    // First, try to find a server block that matches both port and server_name
    for (size_t i = 0; i < serverConfigs.size(); ++i) {
        std::cout << "Checking server config " << i << ": port=" << serverConfigs[i].port << std::endl;

        if (serverConfigs[i].port == port) {
            std::cout << "  Port matches" << std::endl;

            // Check if the host matches any of the server_names
            for (size_t j = 0; j < serverConfigs[i].serverNames.size(); ++j) {
                std::cout << "  Checking server_name: '" << serverConfigs[i].serverNames[j] << "'" << std::endl;

                if (serverConfigs[i].serverNames[j] == host) {
                    std::cout << "  Host matches!" << std::endl;
                    return &serverConfigs[i];
                }
            }
        }
    }

    // If no exact match, return the first server block with matching port
    for (size_t i = 0; i < serverConfigs.size(); ++i) {
        if (serverConfigs[i].port == port) {
            return &serverConfigs[i];
        }
    }

    // If no matching port, return the first server block (default)
    if (!serverConfigs.empty()) {
        return &serverConfigs[0];
    }

    return NULL;
}

// Match request path to location block
Server::LocationConfig* Server::matchLocationConfig(ServerConfig* serverConfig, const string& path) {
    if (!serverConfig) {
        return NULL;
    }

    // Find the location block with the longest matching path prefix
    LocationConfig* bestMatch = NULL;
    size_t bestMatchLength = 0;

    for (size_t i = 0; i < serverConfig->locations.size(); ++i) {
        LocationConfig* location = &serverConfig->locations[i];
        string locationPath = location->path;

        // Exact match
        if (path == locationPath) {
            return location;
        }

        // Prefix match (ensure it's a path prefix, not just a string prefix)
        if (path.substr(0, locationPath.length()) == locationPath &&
            (locationPath[locationPath.length() - 1] == '/' || path.length() == locationPath.length() || path[locationPath.length()] == '/')) {
            if (locationPath.length() > bestMatchLength) {
                bestMatch = location;
                bestMatchLength = locationPath.length();
            }
        }
    }

    // If no match found, use the default location (first one)
    if (!bestMatch && !serverConfig->locations.empty()) {
        bestMatch = &serverConfig->locations[0];
    }

    return bestMatch;
}

// Get physical path from request path and location configuration
string Server::getPhysicalPath(const LocationConfig* location, const string& requestPath) {
    std::cout << "Getting physical path for request path: '" << requestPath << "'" << std::endl;

    // Special case for CGI scripts - don't apply overly strict sanitization
    bool isCgiRequest = requestPath.find("/cgi-bin/") == 0;

    // First, check if the request path contains any suspicious patterns
    if (!isCgiRequest && Utils::containsSuspiciousPatterns(requestPath)) {
        std::cout << "Suspicious pattern detected in request path: '" << requestPath << "'" << std::endl;
        return location ? location->root : "/tmp/www";
    }

    // Default root directory if no location is provided
    string rootDir = location ? location->root : "/tmp/www";

    // For security, we'll only allow access to files within the root directory
    // We'll construct a relative path from the request path and then combine it with the root

    // First, URL decode the request path
    string decodedPath = Utils::urlDecode(requestPath);

    // Check if the decoded path contains suspicious patterns
    if (Utils::containsSuspiciousPatterns(decodedPath)) {
        std::cout << "Suspicious pattern detected in decoded path: '" << decodedPath << "'" << std::endl;
        return rootDir;
    }

    // Remove any leading slashes from the decoded path
    while (!decodedPath.empty() && decodedPath[0] == '/') {
        decodedPath = decodedPath.substr(1);
    }

    // Remove any location prefix from the path
    if (location && decodedPath.substr(0, location->path.length()) == location->path) {
        decodedPath = decodedPath.substr(location->path.length());
        // Remove any leading slashes again
        while (!decodedPath.empty() && decodedPath[0] == '/') {
            decodedPath = decodedPath.substr(1);
        }
    }

    // Construct the physical path by combining the root directory with the relative path
    string physicalPath = rootDir;
    if (physicalPath[physicalPath.length() - 1] != '/') {
        physicalPath += "/";
    }
    physicalPath += decodedPath;

    std::cout << "Path before sanitization: '" << physicalPath << "'" << std::endl;

    // Normalize the path to resolve any . or .. components
    string normalizedPath = Utils::normalizePath(physicalPath);

    // Ensure the normalized path is within the root directory
    if (!Utils::isPathSafe(normalizedPath, rootDir)) {
        std::cout << "Path is outside root directory: '" << normalizedPath << "' not in '" << rootDir << "'" << std::endl;
        return rootDir;
    }

    std::cout << "Final sanitized physical path: '" << normalizedPath << "'" << std::endl;
    return normalizedPath;
}

// Check if the HTTP method is allowed for this location
bool Server::isMethodAllowed(const LocationConfig* location, const string& method) {
    if (!location || location->allowedMethods.empty()) {
        // If no methods are specified, allow all
        return true;
    }

    for (size_t i = 0; i < location->allowedMethods.size(); ++i) {
        if (location->allowedMethods[i] == method) {
            return true;
        }
    }

    return false;
}

// Handle redirection if configured
bool Server::handleRedirection(int clientFd, const LocationConfig* location) {
    if (!location || location->redirect.empty()) {
        return false;
    }

    // Parse the redirect value (status_code URL)
    string redirectValue = location->redirect;
    int statusCode = 301; // Default to permanent redirect
    string redirectUrl;

    size_t spacePos = redirectValue.find(' ');
    if (spacePos != string::npos) {
        string codeStr = redirectValue.substr(0, spacePos);
        redirectUrl = redirectValue.substr(spacePos + 1);
        Utils::ft_trim(redirectUrl);

        // Try to parse the status code
        char* endptr = NULL;
        long code = std::strtol(codeStr.c_str(), &endptr, 10);
        if (*endptr == '\0' && code >= 300 && code < 400) {
            statusCode = static_cast<int>(code);
        }
    } else {
        redirectUrl = redirectValue;
    }

    // Send the redirect response
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << (statusCode == 301 ? " Moved Permanently" : " Found") << "\r\n"
             << "Location: " << redirectUrl << "\r\n"
             << "Content-Length: 0\r\n\r\n";

    sendResponse(clientFd, response.str());
    return true;
}

// Handle directory listing (autoindex)
bool Server::handleAutoindex(int clientFd, const LocationConfig* location, const string& physicalPath) {
    if (!location || !location->autoindex) {
        return false;
    }

    // Check if the path is a directory
    struct stat pathStat;
    if (stat(physicalPath.c_str(), &pathStat) != 0 || !S_ISDIR(pathStat.st_mode)) {
        return false;
    }

    // Try to open the directory
    DIR* dir = opendir(physicalPath.c_str());
    if (!dir) {
        return false;
    }

    // Generate HTML directory listing
    std::ostringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html>\n"
         << "<head>\n"
         << "    <title>Index of " << location->path << "</title>\n"
         << "    <style>\n"
         << "        body { font-family: Arial, sans-serif; margin: 20px; }\n"
         << "        h1 { border-bottom: 1px solid #ccc; padding-bottom: 10px; }\n"
         << "        table { border-collapse: collapse; width: 100%; }\n"
         << "        th, td { text-align: left; padding: 8px; }\n"
         << "        tr:nth-child(even) { background-color: #f2f2f2; }\n"
         << "        a { text-decoration: none; color: #0366d6; }\n"
         << "        a:hover { text-decoration: underline; }\n"
         << "    </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "    <h1>Index of " << location->path << "</h1>\n"
         << "    <table>\n"
         << "        <tr>\n"
         << "            <th>Name</th>\n"
         << "            <th>Last Modified</th>\n"
         << "            <th>Size</th>\n"
         << "        </tr>\n";

    // Add parent directory link
    html << "        <tr>\n"
         << "            <td><a href=\"../\">../</a></td>\n"
         << "            <td>-</td>\n"
         << "            <td>-</td>\n"
         << "        </tr>\n";

    // Read directory entries
    struct dirent* entry;
    std::vector<string> entries;

    while ((entry = readdir(dir)) != NULL) {
        string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        entries.push_back(name);
    }

    // Sort entries alphabetically
    std::sort(entries.begin(), entries.end());

    // Add entries to the HTML
    for (size_t i = 0; i < entries.size(); ++i) {
        // Sanitize the entry name to prevent directory traversal
        string sanitizedEntry = entries[i];

        // Construct the full path for the entry
        string entryPath = physicalPath + "/" + sanitizedEntry;

        // Ensure the entry path is within the allowed directory
        if (!Utils::isPathSafe(entryPath, physicalPath)) {
            std::cout << "Skipping unsafe path: " << entryPath << std::endl;
            continue; // Skip entries that would escape the directory
        }

        struct stat entryStat;

        if (stat(entryPath.c_str(), &entryStat) == 0) {
            // Format the last modified time
            char timeBuffer[80];
            struct tm* timeInfo = localtime(&entryStat.st_mtime);
            strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", timeInfo);

            // Format the size
            string sizeStr;
            if (S_ISDIR(entryStat.st_mode)) {
                sizeStr = "-";
                entries[i] += "/";
            } else {
                if (entryStat.st_size < 1024) {
                    std::ostringstream ss;
                    ss << entryStat.st_size << " B";
                    sizeStr = ss.str();
                } else if (entryStat.st_size < 1024 * 1024) {
                    std::ostringstream ss;
                    ss << entryStat.st_size / 1024 << " KB";
                    sizeStr = ss.str();
                } else {
                    std::ostringstream ss;
                    ss << entryStat.st_size / (1024 * 1024) << " MB";
                    sizeStr = ss.str();
                }
            }

            html << "        <tr>\n"
                 << "            <td><a href=\"" << entries[i] << "\">" << entries[i] << "</a></td>\n"
                 << "            <td>" << timeBuffer << "</td>\n"
                 << "            <td>" << sizeStr << "</td>\n"
                 << "        </tr>\n";
        }
    }

    html << "    </table>\n"
         << "</body>\n"
         << "</html>\n";

    closedir(dir);

    // Send the directory listing
    std::string body = html.str();
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: text/html\r\n"
             << "Content-Length: " << body.length() << "\r\n\r\n"
             << body;

    sendResponse(clientFd, response.str());
    return true;
}

bool Server::initialize(int /* defaultPort */) {
    // Extract unique ports from server configurations
    std::set<int> ports;
    for (size_t i = 0; i < serverConfigs.size(); ++i) {
        ports.insert(serverConfigs[i].port);
    }

    if (ports.empty()) {
        std::cerr << "No ports specified in configuration" << std::endl;
        return false;
    }

    // Initialize a socket for each port
    for (std::set<int>::iterator it = ports.begin(); it != ports.end(); ++it) {
        int port = *it;

        // Create socket
        int serverSocket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
            std::cerr << "Failed to create socket for port " << port << std::endl;
            continue; // Try next port
        }

        // Set socket options
        int opt = 1;
        if (::setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "Failed to set socket options for port " << port << std::endl;
            ::close(serverSocket);
            continue; // Try next port
        }

        // Set non-blocking
        if (!setNonBlocking(serverSocket)) {
            std::cerr << "Failed to set non-blocking mode for port " << port << std::endl;
            ::close(serverSocket);
            continue; // Try next port
        }

        // Bind socket
        struct sockaddr_in serverAddr;
        std::memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = ::htons(static_cast<uint16_t>(port));

        if (::bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Failed to bind to port " << port << ": " << strerror(errno) << std::endl;
            ::close(serverSocket);
            continue; // Try next port
        }

        // Listen
        if (::listen(serverSocket, SOMAXCONN) < 0) {
            std::cerr << "Failed to listen on port " << port << ": " << strerror(errno) << std::endl;
            ::close(serverSocket);
            continue; // Try next port
        }

        // Add to server sockets list
        serverSockets.push_back(serverSocket);
        socketPorts[serverSocket] = port;

        // Initialize poll with server socket
        pollfd serverPoll = {serverSocket, POLLIN, 0};
        fds.push_back(serverPoll);

        std::cout << "Server initialized on port " << port << std::endl;
    }

    if (serverSockets.empty()) {
        std::cerr << "Failed to initialize any server sockets" << std::endl;
        return false;
    }

    running = true;
    return true;
}

void Server::run() {
    while (running) {
        // Use a shorter poll timeout to allow for regular timeout checks
        int ready = ::poll(fds.data(), fds.size(), 1000); // 1 second timeout

        if (ready < 0) {
            if (errno == EINTR) continue;
            std::cerr << "Poll failed: " << strerror(errno) << std::endl;
            break;
        }

        // Check for client timeouts
        checkTimeouts();

        // No events, continue polling
        if (ready == 0) continue;

        // Check each file descriptor
        for (size_t i = 0; i < fds.size() && ready > 0; ++i) {
            if (fds[i].revents == 0)
                continue;

            ready--;

            // Check if this is a server socket
            bool isServerSocket = false;
            for (size_t j = 0; j < serverSockets.size(); ++j) {
                if (fds[i].fd == serverSockets[j]) {
                    isServerSocket = true;
                    if (fds[i].revents & POLLIN) {
                        handleNewConnection(fds[i].fd);
                    }
                    break;
                }
            }

            if (!isServerSocket) {
                if (fds[i].revents & POLLIN) {
                    handleClientData(fds[i].fd);
                    // Update client activity time
                    updateClientActivity(fds[i].fd);
                }

                if (fds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
                    std::cerr << "Client socket error: " <<
                        (fds[i].revents & POLLHUP ? "POLLHUP " : "") <<
                        (fds[i].revents & POLLERR ? "POLLERR " : "") <<
                        (fds[i].revents & POLLNVAL ? "POLLNVAL" : "") << std::endl;
                    removeClient(fds[i].fd);
                }
            }
        }
    }
}

void Server::handleNewConnection(int serverSocket) {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    int clientFd = ::accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientFd < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN)
            std::cerr << "Accept failed: " << strerror(errno) << std::endl;
        return;
    }

    if (!setNonBlocking(clientFd)) {
        std::cerr << "Failed to set client socket non-blocking: " << strerror(errno) << std::endl;
        ::close(clientFd);
        return;
    }

    // Check if we've reached the connection limit
    if (fds.size() >= MAX_CLIENTS) {
        std::cerr << "Max clients reached " << fds.size() << "/" << MAX_CLIENTS << std::endl;

        // Send a 503 Service Unavailable response before closing
        const char* response = "HTTP/1.1 503 Service Unavailable\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: 35\r\n"
                              "Connection: close\r\n"
                              "Retry-After: 30\r\n\r\n"
                              "Server too busy, please try again.";

        // Send the response (ignoring errors since we're closing anyway)
        ::send(clientFd, response, strlen(response), 0);
        ::close(clientFd);
        return;
    }

    // Add the client to our data structures
    pollfd clientPoll = {clientFd, POLLIN, 0};
    fds.push_back(clientPoll);
    clientRequests[clientFd] = HttpRequest();

    // Store which port this client connected to
    clientPorts[clientFd] = socketPorts[serverSocket];

    // Initialize the client's activity timestamp
    updateClientActivity(clientFd);

    std::cout << "New client connected: " << ::inet_ntoa(clientAddr.sin_addr)
              << " on port " << socketPorts[serverSocket]
              << " (" << fds.size() - 1 << " clients total)" << std::endl;
}

void Server::handleClientData(int clientFd) {
    HttpRequest& request = clientRequests[clientFd];

    // Determine optimal buffer size based on the request
    size_t bufferSize = getOptimalBufferSize(request);

    // Allocate buffer dynamically
    char* buffer = new char[bufferSize];

    ssize_t bytesRead = ::recv(clientFd, buffer, bufferSize - 1, 0);

    if (bytesRead <= 0) {
        if (bytesRead == 0) {
            std::cout << "Client disconnected gracefully" << std::endl;
            removeClient(clientFd);
        } else if (errno != EWOULDBLOCK && errno != EAGAIN) {
            std::cerr << "Error reading from client: " << strerror(errno) << std::endl;
            removeClient(clientFd);
        }
        delete[] buffer;
        return;
    }

    buffer[bytesRead] = '\0';

    // Append new data to the temporary buffer
    request.temporaryBuffer.append(buffer, static_cast<size_t>(bytesRead));

    // Check if the buffer is getting too large (potential DoS attack)
    if (request.temporaryBuffer.length() > MAX_BUFFER_SIZE) {
        std::cerr << "Request too large from client " << clientFd << ": "
                  << request.temporaryBuffer.length() << " bytes" << std::endl;
        sendErrorResponse(clientFd, 413, "Request Entity Too Large", "Request body is too large");
        removeClient(clientFd);
        delete[] buffer;
        return;
    }

    // Free the buffer memory
    delete[] buffer;

    try {
        // If we're still reading headers or starting a new request
        if (request.state == READING_HEADERS) {
            std::istringstream input(request.temporaryBuffer);
            parseRequest(input, request);

            // Check if this is a chunked request
            if (!request.headers["transfer-encoding"].empty() &&
                request.headers["transfer-encoding"] == "chunked") {
                // Chunked requests are now handled directly in parseRequest
                // so we don't need to set any special state here
                request.chunkedTransfer = true;
                request.state = REQUEST_COMPLETE;
            } else if (request.contentLength > 0) {
                // If there's a content-length, we need to read the body
                request.state = READING_BODY;

                // Move any remaining data to the body
                size_t headerEnd = request.temporaryBuffer.find("\r\n\r\n");
                if (headerEnd != string::npos) {
                    request.body = request.temporaryBuffer.substr(headerEnd + 4);
                    request.bytesRead = request.body.length();
                    request.temporaryBuffer.clear();
                }

                // Check if we've read the entire body
                if (request.bytesRead >= request.contentLength) {
                    request.state = REQUEST_COMPLETE;
                }
            } else {
                // No body to read, request is complete
                request.state = REQUEST_COMPLETE;
            }
        }
        // If we're reading the body of a non-chunked request
        else if (request.state == READING_BODY) {
            // We've already appended the data to the temporary buffer
            // Just update the bytes read counter
            request.bytesRead += static_cast<size_t>(bytesRead);

            // Check if we've read the entire body
            if (request.bytesRead >= request.contentLength) {
                // Move data from temporary buffer to body
                request.body = request.temporaryBuffer;
                request.temporaryBuffer.clear();
                request.state = REQUEST_COMPLETE;
            }
        }
        // We no longer need to handle chunked requests here as they're handled in parseRequest

        // If the request is complete, handle it
        if (request.state == REQUEST_COMPLETE) {
            handleRequest(clientFd, request);

            // Reset the request for the next one
            request = HttpRequest();
        }
    } catch (const std::exception& e) {
        std::string body = "Bad Request";
        std::ostringstream response;
        response << "HTTP/1.1 400 Bad Request\r\n"
                << "Content-Length: " << body.length() << "\r\n\r\n"
                << body;
        sendResponse(clientFd, response.str());

        // Reset the request for the next one
        request = HttpRequest();
    }
}

void Server::handleRequest(int clientFd, const HttpRequest& request) {
    // Debug: Print all headers
    std::cout << "Request headers:" << std::endl;
    for (map<string, string>::const_iterator it = request.headers.begin(); it != request.headers.end(); ++it) {
        std::cout << "  " << it->first << ": " << it->second << std::endl;
    }

    // Extract host from headers
    string host = request.headers.count("host") ? request.headers.at("host") : "";
    std::cout << "Extracted host header: '" << host << "'" << std::endl;

    // Only block access to system files and directories if they're explicitly mentioned
    // This allows normal web requests to go through
    string lowerPath = request.path;
    for (size_t i = 0; i < lowerPath.length(); ++i) {
        lowerPath[i] = static_cast<char>(tolower(static_cast<unsigned char>(lowerPath[i])));
    }

    // Check for sensitive file patterns in the request path
    if (lowerPath.find("/etc/passwd") != string::npos ||
        lowerPath.find("/etc/shadow") != string::npos ||
        lowerPath.find("/proc/") != string::npos ||
        lowerPath.find("/dev/") != string::npos ||
        lowerPath.find("/sys/") != string::npos ||
        lowerPath.find("/boot/") != string::npos ||
        lowerPath.find("/root/") != string::npos) {
        std::string body = "Access denied to system files and directories";
        sendErrorResponse(clientFd, 403, "Forbidden", body);
        return;
    }

    // Block only obvious path traversal attempts
    if (request.path.find("../") != string::npos ||
        request.path.find("/..") != string::npos ||
        request.path.find("%2e%2e") != string::npos) {
        std::string body = "Path traversal attempt detected";
        sendErrorResponse(clientFd, 403, "Forbidden", body);
        return;
    }

    // Special case for CGI scripts - don't apply overly strict sanitization
    if (request.path.find("/cgi-bin/") == 0) {
        std::cout << "CGI script request detected, bypassing strict path sanitization" << std::endl;
    }
    // Remove port from host if present
    size_t colonPos = host.find(':');
    if (colonPos != string::npos) {
        host = host.substr(0, colonPos);
    }

    // Get the port from the clientPorts map
    int port = 8080; // Default port
    if (clientPorts.find(clientFd) != clientPorts.end()) {
        port = clientPorts[clientFd];
    }

    std::cout << "Using port: " << port << " for client " << clientFd << std::endl;

    // Match request to server block
    ServerConfig* serverConfig = matchServerConfig(request, host, port);
    if (!serverConfig) {
        std::string body = "No matching server configuration";
        sendErrorResponse(clientFd, 500, "Internal Server Error", body);
        return;
    }

    // Match request to location block
    LocationConfig* locationConfig = matchLocationConfig(serverConfig, request.path);
    if (!locationConfig) {
        std::string body = "No matching location configuration";
        sendErrorResponse(clientFd, 500, "Internal Server Error", body);
        return;
    }

    // Check if the method is allowed
    if (!isMethodAllowed(locationConfig, request.method)) {
        std::string body = "The requested method '" + request.method + "' is not allowed for this resource. Allowed methods: " +
                          Utils::ft_join(locationConfig->allowedMethods, ", ");

        // Generate a pretty HTML error page
        string htmlErrorPage = generateErrorPage(405, "Method Not Allowed", body);

        std::ostringstream response;
        response << "HTTP/1.1 405 Method Not Allowed\r\n"
                << "Content-Type: text/html\r\n"
                << "Content-Length: " << htmlErrorPage.length() << "\r\n"
                << "Allow: " << Utils::ft_join(locationConfig->allowedMethods, ", ") << "\r\n\r\n"
                << htmlErrorPage;

        sendResponse(clientFd, response.str());
        return;
    }

    // Check if the request body exceeds the limit
    if (locationConfig->clientMaxBodySize > 0 && request.contentLength > locationConfig->clientMaxBodySize) {
        std::string body = "Request entity too large";
        sendErrorResponse(clientFd, 413, "Request Entity Too Large", body);
        return;
    }

    // Handle redirection if configured
    if (handleRedirection(clientFd, locationConfig)) {
        return;
    }

    // Get the physical path
    string physicalPath = getPhysicalPath(locationConfig, request.path);

    // Handle the request based on the method
    if (request.method == "GET") {
        handleGetRequest(clientFd, request, locationConfig, physicalPath);
    } else if (request.method == "POST") {
        handlePostRequest(clientFd, request, locationConfig, physicalPath);
    } else if (request.method == "DELETE") {
        handleDeleteRequest(clientFd, request, physicalPath);
    } else {
        std::string body = "Method Not Supported";
        sendErrorResponse(clientFd, 501, "Not Implemented", body);
    }
}

void Server::handleGetRequest(int clientFd, const HttpRequest& request, LocationConfig* location, const string& physicalPath) {
    // Additional security check - block access to sensitive system files
    string lowerPath = physicalPath;
    for (size_t i = 0; i < lowerPath.length(); ++i) {
        lowerPath[i] = static_cast<char>(tolower(static_cast<unsigned char>(lowerPath[i])));
    }

    // Check for sensitive file patterns
    if (lowerPath.find("/etc/passwd") != string::npos ||
        lowerPath.find("/etc/shadow") != string::npos ||
        lowerPath.find("/proc/") != string::npos ||
        lowerPath.find("/dev/") != string::npos ||
        lowerPath.find("/sys/") != string::npos ||
        lowerPath.find("/boot/") != string::npos ||
        lowerPath.find("/root/") != string::npos ||
        lowerPath.find("/home/") != string::npos) {
        std::string body = "Access denied to system files";
        sendErrorResponse(clientFd, 403, "Forbidden", body);
        return;
    }

    // Check if this is a CGI request
    string cgiHandler;
    if (isCgiRequest(physicalPath, location, cgiHandler)) {
        handleCgiRequest(clientFd, request, location, physicalPath, cgiHandler);
        return;
    }
    // Check if the path is a directory
    struct stat pathStat;
    if (stat(physicalPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
        // Try to find an index file
        bool indexFound = false;
        string indexPath;

        if (location && !location->index.empty()) {
            for (size_t i = 0; i < location->index.size(); ++i) {
                indexPath = physicalPath;
                if (indexPath[indexPath.length() - 1] != '/') {
                    indexPath += "/";
                }
                indexPath += location->index[i];

                if (::access(indexPath.c_str(), F_OK) == 0) {
                    indexFound = true;
                    break;
                }
            }
        }

        if (indexFound) {
            // Serve the index file
            std::ifstream file(indexPath.c_str(), std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                std::string body = "Error opening index file";
                sendErrorResponse(clientFd, 500, "Internal Server Error", body);
                return;
            }

            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            std::vector<char> buffer(static_cast<size_t>(size));
            if (file.read(buffer.data(), size)) {
                std::ostringstream response;
                response << "HTTP/1.1 200 OK\r\n"
                        << "Content-Length: " << size << "\r\n"
                        << "Content-Type: " << getContentType(indexPath) << "\r\n"
                        << "\r\n";

                // Send headers first
                sendResponse(clientFd, response.str());

                // Then send file content
                size_t totalSent = 0;
                while (totalSent < buffer.size()) {
                    ssize_t sent = ::send(clientFd, buffer.data() + totalSent,
                                      buffer.size() - totalSent, 0);
                    if (sent < 0) {
                        if (errno != EWOULDBLOCK && errno != EAGAIN) {
                            removeClient(clientFd);
                            return;
                        }
                        continue;
                    }
                    totalSent += static_cast<size_t>(sent);
                }
            } else {
                std::string body = "Error reading index file";
                sendErrorResponse(clientFd, 500, "Internal Server Error", body);
            }
        } else if (location && location->autoindex) {
            // Generate directory listing
            handleAutoindex(clientFd, location, physicalPath);
        } else {
            // No index file and autoindex is off
            std::string body = "Directory listing not allowed";
            sendErrorResponse(clientFd, 403, "Forbidden", body);
        }

        return;
    }

    // Handle regular file
    std::ifstream file(physicalPath.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::string body = "File not found";
        sendErrorResponse(clientFd, 404, "Not Found", body);
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(static_cast<size_t>(size));
    if (file.read(buffer.data(), size)) {
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n"
                << "Content-Length: " << size << "\r\n"
                << "Content-Type: " << getContentType(physicalPath) << "\r\n"
                << "\r\n";

        // Send headers first
        sendResponse(clientFd, response.str());

        // Then send file content
        size_t totalSent = 0;
        while (totalSent < buffer.size()) {
            ssize_t sent = ::send(clientFd, buffer.data() + totalSent,
                              buffer.size() - totalSent, 0);
            if (sent < 0) {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    removeClient(clientFd);
                    return;
                }
                continue;
            }
            totalSent += static_cast<size_t>(sent);
        }
    } else {
        std::string body = "Error reading file";
        sendErrorResponse(clientFd, 500, "Internal Server Error", body);
    }
}

void Server::handlePostRequest(int clientFd, const HttpRequest& request, LocationConfig* location, const string& physicalPath) {
    // Check if this is a CGI request
    string cgiHandler;
    if (isCgiRequest(physicalPath, location, cgiHandler)) {
        handleCgiRequest(clientFd, request, location, physicalPath, cgiHandler);
        return;
    }
    // Check if the upload path is specified
    string uploadPath = physicalPath;
    if (location && !location->uploadPath.empty()) {
        // Use the configured upload path
        uploadPath = location->uploadPath;

        // Ensure the upload directory exists
        struct stat st;
        if (stat(uploadPath.c_str(), &st) != 0) {
            // Directory doesn't exist, try to create it
            if (::mkdir(uploadPath.c_str(), 0755) != 0) {
                std::string body = "Cannot create upload directory";
                sendErrorResponse(clientFd, 500, "Internal Server Error", body);
                return;
            }
        } else if (!S_ISDIR(st.st_mode)) {
            // Path exists but is not a directory
            std::string body = "Upload path is not a directory";
            sendErrorResponse(clientFd, 500, "Internal Server Error", body);
            return;
        }

        // Append the filename from the original path
        size_t lastSlash = request.path.find_last_of('/');
        if (lastSlash != string::npos && lastSlash < request.path.length() - 1) {
            // Extract the filename from the path
            string filename = request.path.substr(lastSlash + 1);

            // Sanitize the filename to prevent directory traversal
            if (Utils::containsSuspiciousPatterns(filename)) {
                std::cout << "Suspicious pattern detected in filename: '" << filename << "'" << std::endl;

                // Generate a safe filename instead
                time_t now = time(NULL);
                std::ostringstream ss;
                ss << "upload_" << now << ".dat";
                filename = ss.str();
            }

            // Ensure the filename doesn't contain path separators
            size_t pathSepPos;
            while ((pathSepPos = filename.find('/')) != string::npos) {
                filename.replace(pathSepPos, 1, "_");
            }

            // Append the sanitized filename to the upload path
            if (uploadPath[uploadPath.length() - 1] != '/') {
                uploadPath += "/";
            }
            uploadPath += filename;
        } else {
            // Generate a unique filename if none is provided
            time_t now = time(NULL);
            std::ostringstream ss;
            ss << "/upload_" << now << ".dat";
            uploadPath += ss.str();
        }

        // Final safety check - ensure the upload path is within the allowed directory
        if (!Utils::isPathSafe(uploadPath, location->uploadPath)) {
            std::string body = "Invalid upload path";
            sendErrorResponse(clientFd, 400, "Bad Request", body);
            return;
        }
    }

    // Create the file
    std::ofstream file(uploadPath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        std::string body = "Cannot create file";
        sendErrorResponse(clientFd, 500, "Internal Server Error", body);
        return;
    }

    // Write the request body to the file
    file.write(request.body.c_str(), static_cast<std::streamsize>(request.body.length()));
    file.close();

    // Send success response
    std::ostringstream response;
    std::string content = "Created resource at " + uploadPath;
    response << "HTTP/1.1 201 Created\r\n"
            << "Content-Length: " << content.length() << "\r\n"
            << "\r\n"
            << content;

    sendResponse(clientFd, response.str());
}

void Server::handleDeleteRequest(int clientFd, const HttpRequest& /* request */, const string& physicalPath) {
    // Additional safety check - ensure the path is safe
    if (Utils::containsSuspiciousPatterns(physicalPath)) {
        std::string body = "Invalid path";
        sendErrorResponse(clientFd, 400, "Bad Request", body);
        return;
    }

    // Check if the file exists
    if (::access(physicalPath.c_str(), F_OK) != 0) {
        std::string body = "Resource not found";
        sendErrorResponse(clientFd, 404, "Not Found", body);
        return;
    }

    // Try to delete the file
    if (::unlink(physicalPath.c_str()) == 0) {
        std::string body = "Resource deleted";
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n"
                << "Content-Length: " << body.length() << "\r\n\r\n"
                << body;
        sendResponse(clientFd, response.str());
    } else {
        std::string body = "Failed to delete resource";
        sendErrorResponse(clientFd, 500, "Internal Server Error", body);
    }
}

std::string Server::getContentType(const std::string& path) {
    std::string ext = path.substr(path.find_last_of('.') + 1);

    if (ext == "html") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "png") return "image/png";
    if (ext == "gif") return "image/gif";
    if (ext == "txt") return "text/plain";

    return "application/octet-stream";
}

void Server::removeClient(int clientFd) {
    // Remove from poll set
    for (size_t i = 0; i < fds.size(); ++i) {
        if (fds[i].fd == clientFd) {
            fds.erase(fds.begin() + static_cast<std::vector<pollfd>::difference_type>(i));
            break;
        }
    }

    // Clean up associated data structures
    clientRequests.erase(clientFd);
    clientLastActivity.erase(clientFd);
    clientPorts.erase(clientFd);

    // Close the socket
    ::close(clientFd);

    std::cout << "Client " << clientFd << " removed (" << fds.size() - 1 << " clients remaining)" << std::endl;
}

void Server::sendResponse(int clientFd, const std::string& response) {
    size_t totalSent = 0;
    while (totalSent < response.length()) {
        ssize_t sent = ::send(clientFd, response.c_str() + totalSent,
                          response.length() - totalSent, 0);
        if (sent < 0) {
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                removeClient(clientFd);
                return;
            }
            continue;
        }
        totalSent += static_cast<size_t>(sent);
    }
}

void Server::stop() {
    running = false;

    // Close all file descriptors
    std::vector<pollfd>::iterator it;
    for (it = fds.begin(); it != fds.end(); ++it) {
        ::close(it->fd);
    }

    // Clear all data structures
    fds.clear();
    serverSockets.clear();
    socketPorts.clear();
    clientPorts.clear();
    clientRequests.clear();
    clientLastActivity.clear();
}

// Check if a request should be handled by a CGI handler
bool Server::isCgiRequest(const string& path, LocationConfig* location, string& cgiHandler) {
    if (!location) {
        return false;
    }

    // Only check for obvious path traversal attempts in CGI paths
    if (path.find("../") != string::npos ||
        path.find("/..") != string::npos ||
        path.find("%2e%2e") != string::npos) {
        std::cout << "Path traversal attempt detected in CGI path: '" << path << "'" << std::endl;
        return false;
    }

    // Extract the file extension
    size_t dotPos = path.find_last_of('.');
    if (dotPos == string::npos) {
        return false; // No extension
    }

    string extension = path.substr(dotPos);

    // Check if we have a handler for this extension
    if (location->cgiHandlers.find(extension) != location->cgiHandlers.end()) {
        cgiHandler = location->cgiHandlers[extension];

        // Ensure the CGI handler path is safe
        if (Utils::containsSuspiciousPatterns(cgiHandler)) {
            std::cout << "Suspicious pattern detected in CGI handler: '" << cgiHandler << "'" << std::endl;
            return false;
        }

        return true;
    }

    return false;
}

// Handle a CGI request
void Server::handleCgiRequest(int clientFd, const HttpRequest& request, LocationConfig* /* location */, const string& physicalPath, const string& cgiHandler) {
    std::cout << "Handling CGI request: " << physicalPath << " with handler: " << cgiHandler << std::endl;

    // Only check for obvious path traversal attempts in CGI paths
    if (physicalPath.find("../") != string::npos ||
        physicalPath.find("/..") != string::npos ||
        physicalPath.find("%2e%2e") != string::npos) {
        std::string body = "Path traversal attempt detected in CGI path";
        sendErrorResponse(clientFd, 403, "Forbidden", body);
        return;
    }

    // Allow common CGI handlers
    if (cgiHandler != "/usr/bin/python3" && cgiHandler != "/usr/bin/php" && cgiHandler != "/usr/bin/perl" &&
        cgiHandler.find("/usr/bin/") != 0) {
        std::string body = "Invalid CGI handler: " + cgiHandler;
        sendErrorResponse(clientFd, 400, "Bad Request", body);
        return;
    }

    // Check if the script exists and is executable
    if (::access(physicalPath.c_str(), F_OK) != 0) {
        std::string body = "CGI script not found";
        sendErrorResponse(clientFd, 404, "Not Found", body);
        return;
    }

    if (::access(physicalPath.c_str(), X_OK) != 0) {
        std::string body = "CGI script not executable";
        sendErrorResponse(clientFd, 500, "Internal Server Error", body);
        return;
    }

    // Extract PATH_INFO and QUERY_STRING
    string pathInfo = "";
    string queryString = request.rawQuery;

    // Create pipes for communication with the CGI process
    int inputPipe[2];  // Server writes to CGI's stdin
    int outputPipe[2]; // Server reads from CGI's stdout

    if (::pipe(inputPipe) < 0 || ::pipe(outputPipe) < 0) {
        std::string body = "Failed to create pipes";
        sendErrorResponse(clientFd, 500, "Internal Server Error", body);
        return;
    }

    // Fork a child process
    pid_t pid = ::fork();

    if (pid < 0) {
        // Fork failed
        ::close(inputPipe[0]);
        ::close(inputPipe[1]);
        ::close(outputPipe[0]);
        ::close(outputPipe[1]);

        std::string body = "Failed to fork process";
        sendErrorResponse(clientFd, 500, "Internal Server Error", body);
        return;
    }

    if (pid == 0) {
        // Child process (CGI script)

        // Redirect stdin to read from inputPipe
        ::close(inputPipe[1]); // Close write end
        ::dup2(inputPipe[0], STDIN_FILENO);
        ::close(inputPipe[0]);

        // Redirect stdout to write to outputPipe
        ::close(outputPipe[0]); // Close read end
        ::dup2(outputPipe[1], STDOUT_FILENO);
        ::close(outputPipe[1]);

        // Build environment variables
        map<string, string> env = buildCgiEnvironment(request, physicalPath, pathInfo, queryString);

        // Convert environment to char* array for execve
        char** envp = new char*[env.size() + 1];
        int i = 0;

        for (map<string, string>::const_iterator it = env.begin(); it != env.end(); ++it) {
            string envVar = it->first + "=" + it->second;
            envp[i] = new char[envVar.length() + 1];
            std::strcpy(envp[i], envVar.c_str());
            i++;
        }

        envp[i] = NULL; // Null-terminate the array

        // Prepare arguments for execve
        char* const argv[] = {
            const_cast<char*>(cgiHandler.c_str()),
            const_cast<char*>(physicalPath.c_str()),
            NULL
        };

        // Execute the CGI handler with the script as an argument
        ::execve(cgiHandler.c_str(), argv, envp);

        // If execve returns, there was an error
        std::cerr << "execve failed: " << strerror(errno) << std::endl;

        // Clean up environment variables
        for (int j = 0; j < i; j++) {
            delete[] envp[j];
        }
        delete[] envp;

        ::exit(1);
    }

    // Parent process (server)
    ::close(inputPipe[0]);  // Close read end of input pipe
    ::close(outputPipe[1]); // Close write end of output pipe

    // Write request body to the CGI script's stdin
    if (request.method == "POST" || request.method == "PUT") {
        ::write(inputPipe[1], request.body.c_str(), request.body.length());
    }

    ::close(inputPipe[1]); // Close write end after writing

    // Read the CGI script's output
    char buffer[4096];
    string cgiOutput;
    ssize_t bytesRead;

    while ((bytesRead = ::read(outputPipe[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        cgiOutput.append(buffer, static_cast<size_t>(bytesRead));
    }

    ::close(outputPipe[0]); // Close read end after reading

    // Wait for the child process to finish
    int status;
    ::waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        std::ostringstream ss;
        ss << "CGI script execution failed with status " << WEXITSTATUS(status);
        sendErrorResponse(clientFd, 500, "Internal Server Error", ss.str());
        return;
    }

    // Send the CGI output as the HTTP response
    sendCgiResponse(clientFd, cgiOutput);
}

// Parse CGI output and send response to client
void Server::sendCgiResponse(int clientFd, const string& cgiOutput) {
    // Check if the CGI output contains headers
    size_t headerEnd = cgiOutput.find("\r\n\r\n");
    if (headerEnd == string::npos) {
        headerEnd = cgiOutput.find("\n\n");
    }

    if (headerEnd == string::npos) {
        // No headers found, treat the entire output as the body
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n"
                << "Content-Type: text/html\r\n"
                << "Content-Length: " << cgiOutput.length() << "\r\n\r\n"
                << cgiOutput;
        sendResponse(clientFd, response.str());
        return;
    }

    // Parse headers
    string headers = cgiOutput.substr(0, headerEnd);
    string body = cgiOutput.substr(headerEnd + (cgiOutput[headerEnd + 1] == '\n' ? 2 : 4)); // Skip \n\n or \r\n\r\n

    // Default status and content type
    string status = "200 OK";
    string contentType = "text/html";
    map<string, string> responseHeaders;

    // Parse each header line
    size_t pos = 0;
    size_t lineEnd;
    while ((lineEnd = headers.find("\n", pos)) != string::npos) {
        string line = headers.substr(pos, lineEnd - pos);
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line = line.substr(0, line.length() - 1); // Remove trailing \r
        }

        size_t colonPos = line.find(':');
        if (colonPos != string::npos) {
            string name = line.substr(0, colonPos);
            string value = line.substr(colonPos + 1);
            Utils::ft_trim(name);
            Utils::ft_trim(value);

            if (name == "Status") {
                status = value;
            } else if (name == "Content-Type") {
                contentType = value;
            } else {
                responseHeaders[name] = value;
            }
        }

        pos = lineEnd + 1;
    }

    // Construct the HTTP response
    std::ostringstream response;
    response << "HTTP/1.1 " << status << "\r\n"
            << "Content-Type: " << contentType << "\r\n"
            << "Content-Length: " << body.length() << "\r\n";

    // Add any additional headers
    for (map<string, string>::const_iterator it = responseHeaders.begin(); it != responseHeaders.end(); ++it) {
        response << it->first << ": " << it->second << "\r\n";
    }

    response << "\r\n" << body;
    sendResponse(clientFd, response.str());
}

// Build the environment variables for the CGI script
map<string, string> Server::buildCgiEnvironment(const HttpRequest& request, const string& scriptPath, const string& pathInfo, const string& queryString) {
    map<string, string> env;

    // Standard CGI environment variables
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    env["SERVER_SOFTWARE"] = "Webserv/1.0";
    env["SERVER_NAME"] = request.headers.count("host") ? request.headers.at("host") : "localhost";
    env["SERVER_PORT"] = "8080"; // Default port

    // Request-specific variables
    env["REQUEST_METHOD"] = request.method;
    env["REQUEST_URI"] = request.path + (queryString.empty() ? "" : "?" + queryString);
    env["SCRIPT_NAME"] = request.path;
    env["SCRIPT_FILENAME"] = scriptPath;
    env["PATH_INFO"] = pathInfo;
    env["PATH_TRANSLATED"] = scriptPath + pathInfo;
    env["QUERY_STRING"] = queryString;

    // Client information
    env["REMOTE_ADDR"] = "127.0.0.1"; // Default to localhost
    env["REMOTE_HOST"] = "localhost";

    // Headers
    for (map<string, string>::const_iterator it = request.headers.begin(); it != request.headers.end(); ++it) {
        string headerName = it->first;
        string headerValue = it->second;

        // Convert header name to CGI format (HTTP_HEADER_NAME)
        for (size_t i = 0; i < headerName.length(); ++i) {
            headerName[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(headerName[i])));
            if (headerName[i] == '-') {
                headerName[i] = '_';
            }
        }

        env["HTTP_" + headerName] = headerValue;
    }

    // Content information
    if (request.method == "POST" || request.method == "PUT") {
        env["CONTENT_TYPE"] = request.headers.count("content-type") ? request.headers.at("content-type") : "";
        env["CONTENT_LENGTH"] = request.headers.count("content-length") ? request.headers.at("content-length") : "0";
    }

    return env;
}

void Server::checkTimeouts() {
    time_t currentTime = time(NULL);
    std::vector<int> timedOutClients;

    // Find all clients that have timed out
    for (map<int, time_t>::iterator it = clientLastActivity.begin(); it != clientLastActivity.end(); ++it) {
        // Check if this is a client socket (not a server socket)
        bool isServerSocket = false;
        for (size_t i = 0; i < serverSockets.size(); ++i) {
            if (it->first == serverSockets[i]) {
                isServerSocket = true;
                break;
            }
        }

        if (!isServerSocket && (currentTime - it->second) > CONNECTION_TIMEOUT) {
            timedOutClients.push_back(it->first);
        }
    }

    // Remove timed out clients
    for (size_t i = 0; i < timedOutClients.size(); ++i) {
        int clientFd = timedOutClients[i];
        std::cout << "Client " << clientFd << " timed out after " << CONNECTION_TIMEOUT << " seconds" << std::endl;

        // Send a timeout response before closing
        sendErrorResponse(clientFd, 408, "Request Timeout", "Connection timed out");
        removeClient(clientFd);
    }
}

void Server::updateClientActivity(int clientFd) {
    clientLastActivity[clientFd] = time(NULL);
}

string Server::generateErrorPage(int statusCode, const string& statusText, const string& errorMessage) {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html lang=\"en\">\n"
         << "<head>\n"
         << "    <meta charset=\"UTF-8\">\n"
         << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
         << "    <title>" << statusCode << " - " << statusText << "</title>\n"
         << "    <style>\n"
         << "        body {\n"
         << "            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;\n"
         << "            color: #333;\n"
         << "            background-color: #f8f9fa;\n"
         << "            margin: 0;\n"
         << "            padding: 0;\n"
         << "            display: flex;\n"
         << "            justify-content: center;\n"
         << "            align-items: center;\n"
         << "            height: 100vh;\n"
         << "        }\n"
         << "        .error-container {\n"
         << "            background-color: white;\n"
         << "            border-radius: 8px;\n"
         << "            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);\n"
         << "            padding: 40px;\n"
         << "            max-width: 500px;\n"
         << "            text-align: center;\n"
         << "        }\n"
         << "        .error-code {\n"
         << "            font-size: 72px;\n"
         << "            font-weight: bold;\n"
         << "            margin: 0;\n"
         << "            color: #e74c3c;\n"
         << "        }\n"
         << "        .error-title {\n"
         << "            font-size: 24px;\n"
         << "            margin: 10px 0 20px;\n"
         << "        }\n"
         << "        .error-message {\n"
         << "            color: #666;\n"
         << "            margin-bottom: 30px;\n"
         << "            line-height: 1.5;\n"
         << "        }\n"
         << "        .back-button {\n"
         << "            display: inline-block;\n"
         << "            background-color: #3498db;\n"
         << "            color: white;\n"
         << "            padding: 10px 20px;\n"
         << "            border-radius: 4px;\n"
         << "            text-decoration: none;\n"
         << "            font-weight: 500;\n"
         << "            transition: background-color 0.3s;\n"
         << "        }\n"
         << "        .back-button:hover {\n"
         << "            background-color: #2980b9;\n"
         << "        }\n"
         << "    </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "    <div class=\"error-container\">\n"
         << "        <h1 class=\"error-code\">" << statusCode << "</h1>\n"
         << "        <h2 class=\"error-title\">" << statusText << "</h2>\n"
         << "        <p class=\"error-message\">" << errorMessage << "</p>\n"
         << "        <a href=\"/\" class=\"back-button\">Back to Home</a>\n"
         << "    </div>\n"
         << "</body>\n"
         << "</html>";

    return html.str();
}

void Server::sendErrorResponse(int clientFd, int statusCode, const string& statusText, const string& errorMessage) {
    // Generate a pretty HTML error page
    string htmlErrorPage = generateErrorPage(statusCode, statusText, errorMessage);

    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n"
             << "Content-Type: text/html\r\n"
             << "Content-Length: " << htmlErrorPage.length() << "\r\n"
             << "Connection: close\r\n\r\n"
             << htmlErrorPage;

    sendResponse(clientFd, response.str());
}

size_t Server::getOptimalBufferSize(const HttpRequest& request) {
    // Start with the initial buffer size
    size_t bufferSize = INITIAL_BUFFER_SIZE;

    // If we know the content length, use it to determine a better buffer size
    if (request.contentLength > 0) {
        // For large requests, use a larger buffer, but cap it at MAX_BUFFER_SIZE
        if (request.contentLength > INITIAL_BUFFER_SIZE) {
            bufferSize = std::min(request.contentLength, MAX_BUFFER_SIZE);
        }
    }

    // For chunked transfers, use a larger buffer
    if (request.chunkedTransfer) {
        bufferSize = std::min(bufferSize * 2, MAX_BUFFER_SIZE);
    }

    return bufferSize;
}
