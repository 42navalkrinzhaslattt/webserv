#ifndef REPR_HPP
#define REPR_HPP

#include <string>
#include <sstream>

// Function to create a string representation of a value for debugging
template <typename T>
std::string repr(const T &value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// Specialization for std::string to add quotes
inline std::string repr(const std::string &value) {
    std::ostringstream oss;
    oss << '"' << value << '"';
    return oss.str();
}

// Specialization for char* to add quotes
inline std::string repr(const char *value) {
    std::ostringstream oss;
    oss << '"' << value << '"';
    return oss.str();
}

#endif // REPR_HPP
