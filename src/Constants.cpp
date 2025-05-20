#include "Constants.hpp"

namespace Constants {
    const std::string webservVersion = "1.0.0";
    const std::string httpVersion = "HTTP/1.1";
    const std::string helpText =
        "Usage: webserv [options] [config_file]\n"
        "Options:\n"
        "  -h, --help     Show this help message and exit\n"
        "  -v, --version  Show version information and exit\n"
        "  -t             Test configuration and exit\n"
        "  -T             Test configuration, dump it and exit\n"
        "  -c <file>      Use alternative configuration file (can also be specified without -c)\n"
        "  -l <level>     Set log level (FATAL, ERROR, WARN, INFO, DEBUG, TRACE, TRACE2, TRACE3, TRACE4, TRACE5, TRACE6, TRACE7, TRACE8, TRACE9)\n";
}
