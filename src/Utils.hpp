#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

namespace Utils {
    bool isPrefix(const std::string &prefix, const std::string &str);
    size_t convertSizeToBytes(const std::string &sizeStr);
    std::string trim(const std::string &str);
}

#endif // UTILS_HPP
