#include "Utils.hpp"

#include <cctype>
#include <cstdlib>

namespace Utils {
    bool isPrefix(const std::string &prefix, const std::string &str) {
        if (prefix.length() > str.length()) {
            return false;
        }

        return str.substr(0, prefix.length()) == prefix;
    }

    std::string trim(const std::string &str) {
        if (str.empty()) {
            return str;
        }

        size_t first = str.find_first_not_of(" \t\n\r\f\v");
        if (first == std::string::npos) {
            return "";
        }

        size_t last = str.find_last_not_of(" \t\n\r\f\v");
        return str.substr(first, last - first + 1);
    }

    size_t convertSizeToBytes(const std::string &sizeStr) {
        if (sizeStr.empty()) {
            return 0;
        }

        size_t size = 0;
        size_t i = 0;

        // Parse the numeric part
        while (i < sizeStr.length() && std::isdigit(sizeStr[i])) {
            size = size * 10 + (sizeStr[i] - '0');
            ++i;
        }

        // Skip whitespace
        while (i < sizeStr.length() && std::isspace(sizeStr[i])) {
            ++i;
        }

        // Parse the unit
        if (i < sizeStr.length()) {
            char unit = std::tolower(sizeStr[i]);

            switch (unit) {
                case 'k':
                    size *= 1024;
                    break;
                case 'm':
                    size *= 1024 * 1024;
                    break;
                case 'g':
                    size *= 1024 * 1024 * 1024;
                    break;
                default:
                    // No unit or unrecognized unit, use as is
                    break;
            }
        }

        return size;
    }
}
