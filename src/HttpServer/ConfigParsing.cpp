#include "HttpServer.hpp"
#include "Utils.hpp"

#include <fstream>
#include <sstream>
#include <cstring>

void HttpServer::parseConfig(const std::string &configPath) {
    log.info() << "Parsing configuration file: " << configPath << std::endl;

    // Don't clear existing locations
    log.debug() << "_locations address in parseConfig: " << &_locations << std::endl;
    log.debug() << "_locations size in parseConfig: " << _locations.size() << std::endl;

    std::ifstream configFile(configPath.c_str());
    if (!configFile.is_open()) {
        log.error() << "Failed to open configuration file: " << configPath << std::endl;
        return;
    }

    std::string line;
    std::string serverBlock;
    bool inServerBlock = false;

    while (std::getline(configFile, line)) {
        // Remove comments
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        // Trim whitespace
        line = Utils::trim(line);

        // Skip empty lines
        if (line.empty()) {
            continue;
        }

        log.debug() << "Config line: " << line << std::endl;

        // Check for server block start
        if (line == "server {") {
            inServerBlock = true;
            serverBlock = line + "\n";
        }
        // Check for server block end
        else if (line == "}" && inServerBlock && !serverBlock.empty()) {
            // Only end the server block if we're not inside a nested block
            size_t openBraces = 0;
            size_t closeBraces = 0;
            for (size_t i = 0; i < serverBlock.length(); ++i) {
                if (serverBlock[i] == '{') {
                    openBraces++;
                } else if (serverBlock[i] == '}') {
                    closeBraces++;
                }
            }

            // If we have an equal number of open and close braces, we're at the end of the server block
            if (openBraces == closeBraces + 1) {
                inServerBlock = false;
                serverBlock += line + "\n";

                // Parse the server block
                parseServerBlock(serverBlock);

                // Reset server block
                serverBlock = "";
            } else {
                // Otherwise, we're inside a nested block
                serverBlock += line + "\n";
            }
        }
        // Add line to server block
        else if (inServerBlock) {
            serverBlock += line + "\n";
        }
    }

    log.info() << "Parsed " << _serverConfigs.size() << " server configurations" << std::endl;
}

void HttpServer::parseServerBlock(const std::string &serverBlock) {
    log.debug() << "Parsing server block: " << std::endl << serverBlock << std::endl;

    // Create a new server configuration
    ServerConfig serverConfig;
    serverConfig.socket = -1;
    serverConfig.address = "127.0.0.1";
    serverConfig.port = 8080;
    serverConfig.serverName = "localhost";

    std::istringstream iss(serverBlock);
    std::string line;
    bool inLocationBlock = false;
    std::string locationPath;
    std::vector<Arguments> locationDirectives;

    while (std::getline(iss, line)) {
        // Trim whitespace
        line = Utils::trim(line);
        log.debug() << "Parsing line: " << line << std::endl;

        // Skip empty lines and server block start delimiter
        if (line.empty() || line == "server {") {
            continue;
        }

        // Check for location block start
        if (line.find("location ") == 0 && line.find("{") != std::string::npos) {
            log.debug() << "Found location block start: " << line << std::endl;
            inLocationBlock = true;

            // Extract location path
            size_t pathStart = 9; // Length of "location "
            size_t pathEnd = line.find(" {");
            if (pathEnd != std::string::npos) {
                locationPath = line.substr(pathStart, pathEnd - pathStart);
                log.debug() << "Found location block for path: " << locationPath << std::endl;
            } else {
                log.error() << "Failed to extract location path from line: " << line << std::endl;
            }

            // Clear location directives
            locationDirectives.clear();
            continue;
        }

        // Check for location block end
        log.debug() << "Checking if line '" << line << "' is a location block end" << std::endl;
        log.debug() << "inLocationBlock: " << (inLocationBlock ? "true" : "false") << std::endl;
        if (line == "}" && inLocationBlock) {
            log.debug() << "Found location block end: " << line << std::endl;
            log.debug() << "Location path: " << locationPath << std::endl;
            log.debug() << "Location directives size: " << locationDirectives.size() << std::endl;

            // Create a new location context directly
            LocationCtx location;
            location.first = locationPath;
            location.second = locationDirectives;

            // Add the location to the temporary locations vector
            _tempLocations.push_back(location);
            log.debug() << "Added location to _tempLocations: " << locationPath << std::endl;
            log.debug() << "_tempLocations size: " << _tempLocations.size() << std::endl;

            inLocationBlock = false;

            continue;
        }

        // Parse directives
        size_t directiveEnd = line.find(';');
        if (directiveEnd != std::string::npos) {
            std::string directive = line.substr(0, directiveEnd);

            // If in location block, add directive to location
            if (inLocationBlock) {
                // Parse directive and arguments
                std::istringstream directiveStream(directive);
                std::string directiveName;
                directiveStream >> directiveName;

                Arguments args;
                args.push_back(directiveName);

                std::string arg;
                while (directiveStream >> arg) {
                    args.push_back(arg);
                }

                locationDirectives.push_back(args);
                log.debug() << "Added directive to location " << locationPath << ": " << directiveName << std::endl;
            }
            // Otherwise, parse server directive
            else {
                // Parse listen directive
                if (directive.find("listen") == 0) {
                    std::string listenValue = Utils::trim(directive.substr(6));

                    // Parse address and port
                    size_t colonPos = listenValue.find(':');
                    if (colonPos != std::string::npos) {
                        serverConfig.address = listenValue.substr(0, colonPos);
                        serverConfig.port = atoi(listenValue.substr(colonPos + 1).c_str());
                    } else {
                        serverConfig.port = atoi(listenValue.c_str());
                    }
                }
                // Parse server_name directive
                else if (directive.find("server_name") == 0) {
                    std::string serverNameValue = Utils::trim(directive.substr(11));
                    serverConfig.serverName = serverNameValue;
                }
                // Parse other server directives
                else {
                    // Parse directive and arguments
                    std::istringstream directiveStream(directive);
                    std::string directiveName;
                    directiveStream >> directiveName;

                    Arguments args;
                    args.push_back(directiveName);

                    std::string arg;
                    while (directiveStream >> arg) {
                        args.push_back(arg);
                    }

                    // Add directive to default location
                    _defaultLocation.second.push_back(args);
                    log.debug() << "Added directive to default location: " << directiveName << std::endl;
                }
            }
        }
    }

    // Add the locations to the server configuration
    log.debug() << "Adding " << _tempLocations.size() << " locations to server configuration" << std::endl;
    for (std::vector<LocationCtx>::iterator it = _tempLocations.begin(); it != _tempLocations.end(); ++it) {
        serverConfig.locations.push_back(*it);
        log.debug() << "Added location " << it->first << " to server configuration" << std::endl;
    }

    // Clear the temporary locations
    _tempLocations.clear();

    // Add the server configuration
    _serverConfigs.push_back(serverConfig);

    log.debug() << "Added server configuration: " << serverConfig.address << ":" << serverConfig.port
               << " (server_name: " << serverConfig.serverName << ")" << std::endl;
    log.debug() << "Server has " << serverConfig.locations.size() << " locations" << std::endl;
}
