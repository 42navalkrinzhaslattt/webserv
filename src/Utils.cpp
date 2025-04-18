#include "Server.hpp"

inline std::string &Utils::ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
									std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

inline std::string &Utils::rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
						 std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

void	Utils::ft_trim(string &s) {
	ltrim(s);
	rtrim(s);
}

vector<string> Utils::ft_split(const string &str) {
    vector<string> res;
    string token;
    bool inQuote = false;

    for (string::const_iterator it = str.begin(); it != str.end(); ++it) {
        char c = *it;

        // Handle special characters
        if (!inQuote && (c == '{' || c == '}' || c == ';')) {
            if (!token.empty()) {
                res.push_back(token);
                token.clear();
            }
            res.push_back(string(1, c));
            continue;
        }

        // Handle quotes
        if (c == '"') {
            inQuote = !inQuote;
            token += c;
        }
        else if (std::isspace(c) && !inQuote) {
            if (!token.empty()) {
                res.push_back(token);
                token.clear();
            }
        }
        else {
            token += c;
        }
    }

    if (!token.empty()) {
        res.push_back(token);
    }

    if (inQuote) {
        throw std::runtime_error("Unclosed quote in configuration file");
    }

    return res;
}

string Utils::ft_join(const vector<string>& strings, const string& delimiter) {
    string result;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i > 0) {
            result += delimiter;
        }
        result += strings[i];
    }
    return result;
}

// URL decode a string
string Utils::urlDecode(const string& encoded) {
    string result;
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            // Get the hex value
            string hex = encoded.substr(i + 1, 2);
            // Check if the hex characters are valid
            bool validHex = true;
            for (size_t j = 0; j < 2; ++j) {
                char c = hex[j];
                if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
                    validHex = false;
                    break;
                }
            }

            if (validHex) {
                char c = static_cast<char>(strtol(hex.c_str(), NULL, 16));
                result += c;
                i += 2; // Skip the next two characters
            } else {
                // If not valid hex, keep the % character
                result += encoded[i];
            }
        } else if (encoded[i] == '+') {
            result += ' '; // Convert + to space
        } else {
            result += encoded[i];
        }
    }
    return result;
}

// Check if a path contains suspicious patterns that might indicate a path traversal attack
// MODIFIED: Removed checks for file extensions
bool Utils::containsSuspiciousPatterns(const string& path) {
    // Special case for CGI scripts and CGI handlers - don't apply overly strict sanitization
    if (path.find("/cgi-bin/") == 0 || path.find("/tmp/www/cgi-bin/") == 0 ||
        path == "/usr/bin/python3" || path == "/usr/bin/php" || path == "/usr/bin/perl" ||
        path.find("/usr/bin/") == 0) {
        // For CGI scripts, only check for obvious path traversal attempts
        if (path.find("../") != string::npos ||
            path.find("/..") != string::npos ||
            path.find("%2e%2e") != string::npos) {
            return true;
        }
        return false; // Allow CGI scripts and handlers
    }

    // First, URL decode the path to catch encoded attacks
    string decodedPath = urlDecode(path);

    // Try multiple levels of decoding to catch nested encoding attacks
    string doubleDecodedPath = urlDecode(decodedPath);
    string tripleDecodedPath = urlDecode(doubleDecodedPath);

    // Check all decoded versions for suspicious patterns
    const string* pathsToCheck[] = {&path, &decodedPath, &doubleDecodedPath, &tripleDecodedPath};

    for (size_t i = 0; i < 4; ++i) {
        const string& currentPath = *pathsToCheck[i];

        // Check for encoded path traversal sequences
        if (currentPath.find("%2e%2e") != string::npos || // URL encoded ..
            currentPath.find("%2E%2E") != string::npos || // URL encoded .. (uppercase)
            currentPath.find("%252e%252e") != string::npos || // Double URL encoded ..
            currentPath.find("%252E%252E") != string::npos || // Double URL encoded .. (uppercase)
            currentPath.find("%25%32%65%25%32%65") != string::npos) { // Triple encoded
            return true;
        }

        // Check for path traversal with different separators
        if (currentPath.find("../") != string::npos || // Standard path traversal
            currentPath.find("..%2f") != string::npos || // Mixed encoding
            currentPath.find("%2e%2e/") != string::npos || // Encoded dots
            currentPath.find("%2e%2e%2f") != string::npos || // Fully encoded
            currentPath.find("..%5c") != string::npos || // Backslash variant
            currentPath.find("%2e%2e%5c") != string::npos) { // Encoded backslash
            return true;
        }
    }

    // Check for multiple consecutive dots (more than 2) in all decoded paths
    for (size_t i = 0; i < 4; ++i) {
        const string& currentPath = *pathsToCheck[i];
        size_t pos = 0;
        while ((pos = currentPath.find('.', pos)) != string::npos) {
            size_t dotCount = 0;
            while (pos < currentPath.length() && currentPath[pos] == '.') {
                dotCount++;
                pos++;
            }
            if (dotCount > 2) {
                return true; // More than 2 consecutive dots is suspicious
            }
        }

        // Check for other suspicious patterns - MODIFIED to allow legitimate paths and file extensions
        if (currentPath.find("\0") != string::npos) { // Null byte
            return true;
        }
    }

    // Check for attempts to access sensitive files
    string lowerPath = decodedPath;
    for (size_t i = 0; i < lowerPath.length(); ++i) {
        lowerPath[i] = static_cast<char>(tolower(static_cast<unsigned char>(lowerPath[i])));
    }

    // Check for sensitive file patterns
    if (lowerPath.find("/etc/passwd") != string::npos ||
        lowerPath.find("/etc/shadow") != string::npos ||
        lowerPath.find("/proc/self") != string::npos ||
        lowerPath.find("/proc/") != string::npos ||
        lowerPath.find("/dev/") != string::npos ||
        lowerPath.find("/sys/") != string::npos ||
        lowerPath.find("/boot/") != string::npos ||
        lowerPath.find("/root/") != string::npos ||
        lowerPath.find("/home/") != string::npos) {
        return true;
    }

    return false;
}

// Normalize a path by resolving . and .. components
string Utils::normalizePath(const string& path) {
    // If the path is empty, return /
    if (path.empty()) {
        return "/";
    }

    // First, URL decode the path
    string decodedPath = urlDecode(path);

    // Split the path into components
    vector<string> components;
    string component;

    // Ensure the path starts with a slash for absolute paths
    size_t start = 0;
    bool isAbsolute = false;

    if (decodedPath[0] == '/') {
        isAbsolute = true;
        start = 1;
    }

    // Split the path by slashes
    for (size_t i = start; i <= decodedPath.length(); ++i) {
        if (i == decodedPath.length() || decodedPath[i] == '/') {
            if (!component.empty()) {
                if (component == ".") {
                    // Ignore . components
                } else if (component == "..") {
                    // Go up one level for .. components
                    if (!components.empty()) {
                        components.pop_back();
                    }
                } else {
                    // Add regular components
                    components.push_back(component);
                }
                component.clear();
            }
        } else {
            component += decodedPath[i];
        }
    }

    // Reconstruct the normalized path
    string normalizedPath;
    if (isAbsolute) {
        normalizedPath = "/";
    }

    for (size_t i = 0; i < components.size(); ++i) {
        if (i > 0 || !isAbsolute) {
            normalizedPath += "/";
        }
        normalizedPath += components[i];
    }

    // If the path is empty, return /
    if (normalizedPath.empty()) {
        return "/";
    }

    return normalizedPath;
}

// Check if a path is safe (within the root directory)
bool Utils::isPathSafe(const string& path, const string& rootDir) {
    // Special case for CGI scripts - they're always considered safe if they're in the cgi-bin directory
    if (path.find("/cgi-bin/") != string::npos || path.find("/tmp/www/cgi-bin/") != string::npos) {
        // For CGI scripts, only check for obvious path traversal attempts
        if (path.find("../") != string::npos ||
            path.find("/...") != string::npos ||
            path.find("%2e%2e") != string::npos) {
            return false;
        }
        return true; // Allow CGI scripts
    }

    // Check for obvious path traversal attempts
    if (path.find("../") != string::npos ||
        path.find("/...") != string::npos ||
        path.find("%2e%2e") != string::npos ||
        rootDir.find("../") != string::npos ||
        rootDir.find("/...") != string::npos ||
        rootDir.find("%2e%2e") != string::npos) {
        return false;
    }

    // Normalize both paths
    string normalizedPath = normalizePath(path);
    string normalizedRootDir = normalizePath(rootDir);

    // Ensure the normalized root ends with a slash
    if (normalizedRootDir[normalizedRootDir.length() - 1] != '/') {
        normalizedRootDir += "/";
    }

    // Check if the normalized path starts with the normalized root
    if (normalizedPath.length() < normalizedRootDir.length()) {
        return false; // Path is too short to be within the root
    }

    // Check if the normalized path starts with the normalized root
    if (normalizedPath.substr(0, normalizedRootDir.length()) != normalizedRootDir) {
        return false; // Path doesn't start with the root
    }

    // Additional check: ensure there's no ".." after the root prefix
    string relativePath = normalizedPath.substr(normalizedRootDir.length());
    if (relativePath.find("../") != string::npos || relativePath.find("/...") != string::npos) {
        return false; // Found suspicious pattern in the relative path
    }

    return true;
}

// Sanitize a path to ensure it's safe
string Utils::sanitizePath(const string& path, const string& rootDir) {
    // Special case for CGI scripts - don't apply overly strict sanitization
    if (path.find("/cgi-bin/") == 0 || path.find("/tmp/www/cgi-bin/") == 0) {
        // For CGI scripts, only check for obvious path traversal attempts
        if (path.find("../") != string::npos ||
            path.find("/...") != string::npos ||
            path.find("%2e%2e") != string::npos) {
            std::cout << "Path traversal attempt detected in CGI path: '" << path << "'" << std::endl;
            return rootDir;
        }

        // URL decode the path
        string decodedPath = urlDecode(path);

        // Normalize the path
        string normalizedPath = normalizePath(decodedPath);

        return normalizedPath;
    }

    // For non-CGI paths, apply basic sanitization
    // Check for obvious path traversal attempts
    if (path.find("../") != string::npos ||
        path.find("/...") != string::npos ||
        path.find("%2e%2e") != string::npos) {
        std::cout << "Path traversal attempt detected in path: '" << path << "'" << std::endl;
        return rootDir; // Return the root directory if the path is suspicious
    }

    // URL decode the path
    string decodedPath = urlDecode(path);

    // Check for obvious path traversal attempts in decoded path
    if (decodedPath.find("../") != string::npos ||
        decodedPath.find("/...") != string::npos ||
        decodedPath.find("%2e%2e") != string::npos) {
        std::cout << "Path traversal attempt detected in decoded path: '" << decodedPath << "'" << std::endl;
        return rootDir; // Return the root directory if the decoded path is suspicious
    }

    // Normalize the path
    string normalizedPath = normalizePath(decodedPath);

    // Check for obvious path traversal attempts in normalized path
    if (normalizedPath.find("../") != string::npos ||
        normalizedPath.find("/...") != string::npos ||
        normalizedPath.find("%2e%2e") != string::npos) {
        std::cout << "Path traversal attempt detected in normalized path: '" << normalizedPath << "'" << std::endl;
        return rootDir; // Return the root directory if the normalized path is suspicious
    }

    // Ensure the path is within the root directory
    if (!isPathSafe(normalizedPath, rootDir)) {
        std::cout << "Path is outside root directory: '" << normalizedPath << "' not in '" << rootDir << "'" << std::endl;
        return rootDir; // Return the root directory if the path is outside the root
    }

    return normalizedPath;
}
