#include "HttpServer.hpp"
#include "Utils.hpp"

const LocationCtx &HttpServer::requestToLocation(int clientSocket __attribute__((unused)), const HttpRequest &request) const {
    log.debug() << "Finding location for request path: " << request.path << std::endl;

    // Extract path without query string
    std::string path = request.path;
    size_t queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        path = path.substr(0, queryPos);
    }

    // Make sure path starts with a slash
    if (path.empty() || path[0] != '/') {
        path = "/" + path;
    }

    // Get the server configuration for this request
    // For now, just use the first server configuration
    const ServerConfig &serverConfig = _serverConfigs[0];

    // Debug: Print all locations
    log.debug() << "Available locations:" << std::endl;
    for (std::vector<LocationCtx>::const_iterator it = serverConfig.locations.begin(); it != serverConfig.locations.end(); ++it) {
        log.debug() << "  - " << it->first << std::endl;
    }

    // Print the address of the locations vector
    log.debug() << "Server locations size: " << serverConfig.locations.size() << std::endl;

    // Find the best matching location
    const LocationCtx *bestMatch = NULL;
    size_t bestMatchLength = 0;

    for (std::vector<LocationCtx>::const_iterator it = serverConfig.locations.begin(); it != serverConfig.locations.end(); ++it) {
        const LocationCtx &location = *it;
        std::string locationPath = location.first;

        // Make sure location path starts with a slash
        if (locationPath.empty() || locationPath[0] != '/') {
            locationPath = "/" + locationPath;
        }

        // Check if location path is a prefix of request path
        log.debug() << "Checking if location path '" << locationPath << "' is a prefix of request path '" << path << "'" << std::endl;
        if (Utils::isPrefix(locationPath, path)) {
            log.debug() << "Location path '" << locationPath << "' is a prefix of request path '" << path << "'" << std::endl;
            // If this location is a better match than the current best match, update the best match
            if (locationPath.length() > bestMatchLength) {
                bestMatch = &location;
                bestMatchLength = locationPath.length();
                log.debug() << "Updated best match to '" << locationPath << "' with length " << bestMatchLength << std::endl;
            }
        }
    }

    // If no match found, use the default location
    if (bestMatch == NULL) {
        log.debug() << "No matching location found, using default location" << std::endl;
        return _defaultLocation;
    }

    log.debug() << "Found matching location: " << bestMatch->first << std::endl;
    return *bestMatch;
}

void HttpServer::addLocation(const std::string &path, const std::vector<Arguments> &directives) {
    log.debug() << "Adding location: " << path << std::endl;

    // Create a new location context
    LocationCtx location;
    location.first = path;
    location.second = directives;

    // Add the location to the list of locations
    _locations.push_back(location);

    log.debug() << "Added location: " << path << " with " << directives.size() << " directives" << std::endl;
    log.debug() << "Total locations: " << _locations.size() << std::endl;

    // Print all locations
    log.debug() << "All locations:" << std::endl;
    for (std::vector<LocationCtx>::const_iterator it = _locations.begin(); it != _locations.end(); ++it) {
        log.debug() << "  - " << it->first << std::endl;
    }

    // Print the address of the _locations vector
    log.debug() << "_locations vector address: " << &_locations << std::endl;
}

void HttpServer::initDefaultLocation() {
    log.debug() << "Initializing default location" << std::endl;

    // Set the path for the default location
    _defaultLocation.first = "/";

    // Add default directives
    Arguments rootArgs;
    rootArgs.push_back("root");
    rootArgs.push_back("html/default");
    _defaultLocation.second.push_back(rootArgs);

    Arguments indexArgs;
    indexArgs.push_back("index");
    indexArgs.push_back("index.html");
    _defaultLocation.second.push_back(indexArgs);

    Arguments autoindexArgs;
    autoindexArgs.push_back("autoindex");
    autoindexArgs.push_back("on");
    _defaultLocation.second.push_back(autoindexArgs);

    Arguments clientMaxBodySizeArgs;
    clientMaxBodySizeArgs.push_back("client_max_body_size");
    clientMaxBodySizeArgs.push_back("10m");
    _defaultLocation.second.push_back(clientMaxBodySizeArgs);

    log.debug() << "Default location initialized with " << _defaultLocation.second.size() << " directives" << std::endl;
}
