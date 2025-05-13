#include "HttpServer.hpp"
#include "Constants.hpp"
#include "Exceptions.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>

HttpServer::HttpServer(const std::string &configPath, Logger &log, bool onlyCheckConfig)
    : log(log), _httpVersionString(Constants::httpVersion), _running(false) {
    log.info() << "Initializing HTTP server with config file: " << configPath << std::endl;
    log.debug() << "_locations address in constructor: " << &_locations << std::endl;
    log.debug() << "_locations size in constructor: " << _locations.size() << std::endl;

    if (onlyCheckConfig) {
        log.info() << "Configuration check successful" << std::endl;
        throw OnlyCheckConfigException();
    }

    // Parse the configuration file
    parseConfig(configPath);

    // Set up CGI extensions
    _cgiExtensions[".py"] = "/usr/bin/python3";
    _cgiExtensions[".pl"] = "/usr/bin/perl";
    _cgiExtensions[".php"] = "/usr/bin/php";
    _cgiExtensions[".sh"] = "/bin/bash";

    // Initialize MIME types
    initMimeTypes();

    // Initialize HTTP status texts
    initStatusTexts();

    // Initialize default location
    initDefaultLocation();

    // Initialize timeouts
    initTimeouts();

    // If no server configurations were parsed, add a default one
    if (_serverConfigs.empty()) {
        ServerConfig defaultConfig;
        defaultConfig.socket = -1;
        defaultConfig.address = "127.0.0.1";
        defaultConfig.port = 8080;
        defaultConfig.serverName = "localhost";
        _serverConfigs.push_back(defaultConfig);
    }
}

HttpServer::~HttpServer() {
    log.info() << "Shutting down HTTP server" << std::endl;

    // Close all open sockets
    for (std::vector<ServerConfig>::iterator it = _serverConfigs.begin(); it != _serverConfigs.end(); ++it) {
        if (it->socket > 0) {
            close(it->socket);
        }
    }

    for (std::set<int>::iterator it = _clientSockets.begin(); it != _clientSockets.end(); ++it) {
        close(*it);
    }
}

bool HttpServer::setupServerSocket() {

    for (std::vector<ServerConfig>::iterator it = _serverConfigs.begin(); it != _serverConfigs.end(); ++it) {
        // Create socket
        it->socket = socket(AF_INET, SOCK_STREAM, 0);
        if (it->socket < 0) {
            log.error() << "Failed to create socket for " << it->address << ":" << it->port << std::endl;
            continue;
        }

        // Set socket options
        int opt = 1;
        if (setsockopt(it->socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            log.error() << "Failed to set socket options for " << it->address << ":" << it->port << std::endl;
            close(it->socket);
            it->socket = -1;
            continue;
        }

        // Set non-blocking mode
        int flags = fcntl(it->socket, F_GETFL, 0);
        if (flags < 0) {
            log.error() << "Failed to get socket flags for " << it->address << ":" << it->port << std::endl;
            close(it->socket);
            it->socket = -1;
            continue;
        }
        if (fcntl(it->socket, F_SETFL, flags | O_NONBLOCK) < 0) {
            log.error() << "Failed to set non-blocking mode for " << it->address << ":" << it->port << std::endl;
            close(it->socket);
            it->socket = -1;
            continue;
        }

        // Bind socket
        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(it->address.c_str());
        address.sin_port = htons(it->port);

        if (bind(it->socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
            log.error() << "Failed to bind socket for " << it->address << ":" << it->port << std::endl;
            close(it->socket);
            it->socket = -1;
            continue;
        }

        // Listen for connections with a larger backlog for Siege testing
        if (listen(it->socket, 128) < 0) {
            log.error() << "Failed to listen on socket for " << it->address << ":" << it->port << std::endl;
            close(it->socket);
            it->socket = -1;
            continue;
        }

        log.info() << "Server listening on " << it->address << ":" << it->port << " (server_name: " << it->serverName << ")" << std::endl;
    }

    // Check if at least one server socket was set up successfully
    for (std::vector<ServerConfig>::iterator it = _serverConfigs.begin(); it != _serverConfigs.end(); ++it) {
        if (it->socket > 0) {
            return true;
        }
    }

    return false;
}
