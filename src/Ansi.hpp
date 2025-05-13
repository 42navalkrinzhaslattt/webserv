#ifndef ANSI_HPP
#define ANSI_HPP

#include <string>

namespace ansi {
    std::string red(const std::string &text);
    std::string green(const std::string &text);
    std::string yellow(const std::string &text);
    std::string blue(const std::string &text);
    std::string magenta(const std::string &text);
    std::string cyan(const std::string &text);
    std::string bold(const std::string &text);
    std::string underline(const std::string &text);
}

#endif // ANSI_HPP
