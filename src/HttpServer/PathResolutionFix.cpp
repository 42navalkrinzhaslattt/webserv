#include "HttpServer.hpp"
#include "Utils.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

// Helper function to check if a file exists
bool fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

// Helper function to get the current working directory
std::string getCurrentWorkingDir() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        return std::string(cwd);
    }
    return ".";
}

// Modified version of determineDiskPath that handles relative paths correctly
std::string HttpServer::determineDiskPath(const HttpRequest &request, const LocationCtx &location) {
    log.debug() << "Determining disk path for request path: " << request.path << std::endl;

    // Extract the request path without query string
    std::string requestPath = request.path;
    size_t queryPos = requestPath.find('?');
    if (queryPos != std::string::npos) {
        requestPath = requestPath.substr(0, queryPos);
    }

    // Get the location path
    std::string locationPath = location.first;
    log.debug() << "Location path: " << locationPath << std::endl;

    // Get the root path from the location directives
    std::string rootPath = "./";
    if (directiveExists(location.second, "root")) {
        rootPath = getFirstDirective(location.second, "root")[1];
        log.debug() << "Found root directive: " << rootPath << std::endl;
    } else if (directiveExists(_defaultLocation.second, "root")) {
        // If not found in the location, check the default location
        rootPath = getFirstDirective(_defaultLocation.second, "root")[1];
        log.debug() << "Using default root directive: " << rootPath << std::endl;
    }

    // Check if alias directive exists
    if (directiveExists(location.second, "alias")) {
        std::string aliasPath = getFirstDirective(location.second, "alias")[1];
        log.debug() << "Alias directive exists: " << aliasPath << std::endl;

        // If location path is a prefix of request path, replace it with alias path
        if (Utils::isPrefix(locationPath, requestPath)) {
            std::string relativePath = requestPath.substr(locationPath.length());
            std::string diskPath = aliasPath + relativePath;
            log.debug() << "Using alias path: " << diskPath << std::endl;
            return diskPath;
        }
    }

    // Default behavior: use root + request path
    // If the location path is a prefix of the request path, remove it from the request path
    if (Utils::isPrefix(locationPath, requestPath) && locationPath != "/") {
        requestPath = requestPath.substr(locationPath.length());
    }

    std::string diskPath = rootPath + requestPath;
    log.debug() << "Using root path: " << diskPath << std::endl;

    // Check if the file exists
    if (!fileExists(diskPath)) {
        // Try with current working directory
        std::string cwdPath = getCurrentWorkingDir() + "/" + diskPath;
        log.debug() << "File not found, trying with current working directory: " << cwdPath << std::endl;

        if (fileExists(cwdPath)) {
            log.debug() << "File found with current working directory: " << cwdPath << std::endl;
            return cwdPath;
        }

        // Try with absolute path
        if (rootPath[0] != '/') {
            std::string absPath = "/" + diskPath;
            log.debug() << "File not found, trying with absolute path: " << absPath << std::endl;

            if (fileExists(absPath)) {
                log.debug() << "File found with absolute path: " << absPath << std::endl;
                return absPath;
            }
        }
    }

    return diskPath;
}
