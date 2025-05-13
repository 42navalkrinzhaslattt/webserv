#include "HttpServer.hpp"

#include <algorithm>
#include <sstream>
#include <vector>

std::string HttpServer::canonicalizePath(const std::string &path) {
    log.debug() << "Canonicalizing path: " << path << std::endl;

    // Handle empty path
    if (path.empty()) {
        return "/";
    }

    // Split the path into segments
    std::vector<std::string> segments;
    std::string segment;
    std::istringstream pathStream(path);

    // Skip the leading slash
    if (path[0] == '/') {
        pathStream.get();
    }

    // Parse the path segments
    while (std::getline(pathStream, segment, '/')) {
        if (segment.empty() || segment == ".") {
            // Skip empty segments and current directory references
            continue;
        } else if (segment == "..") {
            // Handle parent directory references
            if (!segments.empty()) {
                segments.pop_back();
            }
        } else {
            // Add the segment to the list
            segments.push_back(segment);
        }
    }

    // Reconstruct the path
    std::string canonicalPath = "/";
    for (size_t i = 0; i < segments.size(); ++i) {
        canonicalPath += segments[i];
        if (i < segments.size() - 1) {
            canonicalPath += "/";
        }
    }

    // Preserve trailing slash if the original path had one
    if (path.length() > 1 && path[path.length() - 1] == '/') {
        canonicalPath += "/";
    }

    log.debug() << "Canonicalized path: " << canonicalPath << std::endl;
    return canonicalPath;
}

std::string HttpServer::decodeUri(const std::string &uri) {
    log.debug() << "Decoding URI: " << uri << std::endl;

    std::string decoded;
    decoded.reserve(uri.length());

    for (size_t i = 0; i < uri.length(); ++i) {
        if (uri[i] == '%' && i + 2 < uri.length()) {
            // Handle percent-encoded characters
            int value;
            std::istringstream hex(uri.substr(i + 1, 2));
            if (hex >> std::hex >> value) {
                decoded += static_cast<char>(value);
                i += 2;
            } else {
                decoded += uri[i];
            }
        } else if (uri[i] == '+') {
            // Handle plus signs as spaces
            decoded += ' ';
        } else {
            // Copy other characters as-is
            decoded += uri[i];
        }
    }

    log.debug() << "Decoded URI: " << decoded << std::endl;
    return decoded;
}

std::string HttpServer::normalizeUri(const std::string &uri) {
    log.debug() << "Normalizing URI: " << uri << std::endl;

    // Extract the path and query string
    std::string path = uri;
    std::string query;

    size_t queryPos = uri.find('?');
    if (queryPos != std::string::npos) {
        path = uri.substr(0, queryPos);
        query = uri.substr(queryPos);
    }

    // Decode the path
    std::string decodedPath = decodeUri(path);

    // Canonicalize the path
    std::string canonicalPath = canonicalizePath(decodedPath);

    // Combine the canonical path and query string
    std::string normalizedUri = canonicalPath + query;

    log.debug() << "Normalized URI: " << normalizedUri << std::endl;
    return normalizedUri;
}
