#include "HttpServer.hpp"
#include "DirectoryIndexer.hpp"
#include "Utils.hpp"

#include <sys/stat.h>
#include <unistd.h>

void HttpServer::handleGetRequest(int clientSocket, const HttpRequest &request) {
    log.debug() << "Handling GET request for path: " << request.path << std::endl;

    // Extract query string if present
    std::string query = "";
    std::string cleanPath = request.path;
    size_t queryPos = request.path.find('?');

    if (queryPos != std::string::npos) {
        query = request.path.substr(queryPos + 1);
        cleanPath = request.path.substr(0, queryPos);
    }

    // Get the location context for this request
    const LocationCtx &location = requestToLocation(clientSocket, request);

    // Check if this location has a redirect directive
    if (handleRedirect(clientSocket, request, location)) {
        log.debug() << "Request was redirected" << std::endl;
        return;
    }

    // Check if this is a CGI script
    if (isCgiScript(cleanPath)) {
        log.info() << "Detected CGI script: " << cleanPath << std::endl;
        bool closeConnection = shouldCloseConnection(request);
        executeCgi(clientSocket, cleanPath, request.method, query, "", closeConnection);
        return;
    }

    // Serve static file
    serveStaticFile(clientSocket, request.path, request);
}

std::string HttpServer::determineDiskPath(const HttpRequest &request, const LocationCtx &location) {
    log.debug() << "Determining disk path for request path: " << request.path << std::endl;

    std::string rootPath = "html/default";
    std::string locationPath = location.first;
    std::string requestPath = request.path;

    // Extract query string if present
    size_t queryPos = requestPath.find('?');
    if (queryPos != std::string::npos) {
        requestPath = requestPath.substr(0, queryPos);
    }

    // Check if alias directive exists
    if (directiveExists(location.second, "alias")) {
        std::string aliasPath = getFirstDirective(location.second, "alias")[0];
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
    std::string diskPath = rootPath + requestPath;
    log.debug() << "Using root path: " << diskPath << std::endl;
    return diskPath;
}

bool HttpServer::handleIndexes(int clientSocket, const std::string &diskPath, const HttpRequest &request, const LocationCtx &location) {
    log.debug() << "Trying to handle index files for directory: " << diskPath << std::endl;

    ArgResults indexFiles = getAllDirectives(location.second, "index");
    log.debug() << "Found " << indexFiles.size() << " index directives" << std::endl;

    for (ArgResults::const_iterator it = indexFiles.begin(); it != indexFiles.end(); ++it) {
        for (Arguments::const_iterator indexIt = it->begin(); indexIt != it->end(); ++indexIt) {
            if (indexIt == it->begin()) continue; // Skip the directive name

            std::string indexPath = diskPath + "/" + *indexIt;
            log.debug() << "Checking if index file exists: " << indexPath << std::endl;

            struct stat fileStat;
            if (stat(indexPath.c_str(), &fileStat) == 0 && S_ISREG(fileStat.st_mode)) {
                log.debug() << "Index file " << indexPath << " exists, serving it" << std::endl;
                bool closeConnection = shouldCloseConnection(request);
                return sendFileContent(clientSocket, indexPath, location, 200, "", request.method == "HEAD", closeConnection);
            }
        }
    }

    log.debug() << "No index files found in directory " << diskPath << std::endl;
    return false;
}
