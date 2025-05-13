#include "HttpServer.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <cstring>

bool HttpServer::setNonBlocking(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags < 0) {
        log.error() << "Failed to get socket flags" << std::endl;
        return false;
    }

    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        log.error() << "Failed to set non-blocking mode" << std::endl;
        return false;
    }

    return true;
}

bool HttpServer::setReuseAddr(int socket) {
    int opt = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log.error() << "Failed to set socket options" << std::endl;
        return false;
    }

    return true;
}
