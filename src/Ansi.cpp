#include "Ansi.hpp"

namespace ansi {
    std::string red(const std::string &text) {
        return "\033[31m" + text + "\033[0m";
    }

    std::string green(const std::string &text) {
        return "\033[32m" + text + "\033[0m";
    }

    std::string yellow(const std::string &text) {
        return "\033[33m" + text + "\033[0m";
    }

    std::string blue(const std::string &text) {
        return "\033[34m" + text + "\033[0m";
    }

    std::string magenta(const std::string &text) {
        return "\033[35m" + text + "\033[0m";
    }

    std::string cyan(const std::string &text) {
        return "\033[36m" + text + "\033[0m";
    }

    std::string bold(const std::string &text) {
        return "\033[1m" + text + "\033[0m";
    }

    std::string underline(const std::string &text) {
        return "\033[4m" + text + "\033[0m";
    }
}
