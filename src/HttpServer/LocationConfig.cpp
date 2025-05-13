#include "HttpServer.hpp"
#include "Repr.hpp"

// This file contains the implementation of the location configuration methods

bool HttpServer::directiveExists(const std::vector<Arguments> &directives, const std::string &name) const {
    for (std::vector<Arguments>::const_iterator it = directives.begin(); it != directives.end(); ++it) {
        if (!it->empty() && (*it)[0] == name) {
            return true;
        }
    }
    return false;
}

Arguments HttpServer::getFirstDirective(const std::vector<Arguments> &directives, const std::string &name) const {
    for (std::vector<Arguments>::const_iterator it = directives.begin(); it != directives.end(); ++it) {
        if (!it->empty() && (*it)[0] == name) {
            return *it;
        }
    }

    // Return empty directive if not found
    Arguments emptyArgs;
    emptyArgs.push_back(name);
    emptyArgs.push_back("");
    return emptyArgs;
}

ArgResults HttpServer::getAllDirectives(const std::vector<Arguments> &directives, const std::string &name) const {
    ArgResults results;

    for (std::vector<Arguments>::const_iterator it = directives.begin(); it != directives.end(); ++it) {
        if (!it->empty() && (*it)[0] == name) {
            results.push_back(*it);
        }
    }

    return results;
}

// This method is now implemented in LocationMatching.cpp
