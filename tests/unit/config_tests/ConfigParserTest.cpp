#include "Config.hpp"
#include <iostream>
#include <string>
#include <vector>

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " << message << std::endl; \
            return false; \
        } \
    } while (0)

class ConfigParserTest {
private:
    bool testValidSimpleConfig() {
        try {
            Config config = parseConfig("config_tests/test_configs/valid_simple.conf");
            
            // Check if we have at least one server context
            ASSERT(!config.second.empty(), "Should have at least one server context");
            
            // Get the first server context
            ServerContext& server = config.second[0];
            
            // Check server directives
            bool foundPort = false;
            bool foundRoot = false;
            bool foundServerName = false;
            
            for (Directives::iterator it = server.first.begin(); it != server.first.end(); ++it) {
                if (it->first == "listen") {
                    ASSERT(it->second[0] == "8080", "Port should be 8080");
                    foundPort = true;
                }
                else if (it->first == "root") {
                    ASSERT(it->second[0] == "/tmp/www", "Root should be /tmp/www");
                    foundRoot = true;
                }
                else if (it->first == "server_name") {
                    ASSERT(it->second[0] == "localhost", "Server name should be localhost");
                    foundServerName = true;
                }
            }
            
            ASSERT(foundPort, "Port directive not found");
            ASSERT(foundRoot, "Root directive not found");
            ASSERT(foundServerName, "Server name directive not found");
            
            // Check location context
            ASSERT(!server.second.empty(), "Should have at least one location context");
            LocationContext& location = server.second[0];
            
            ASSERT(location.first == "/", "Location path should be /");
            
            bool foundMethods = false;
            bool foundIndex = false;
            
            for (Directives::iterator it = location.second.begin(); it != location.second.end(); ++it) {
                if (it->first == "allowed_methods") {
                    ASSERT(it->second.size() == 3, "Should have 3 allowed methods");
                    ASSERT(it->second[0] == "GET", "GET should be allowed");
                    ASSERT(it->second[1] == "POST", "POST should be allowed");
                    ASSERT(it->second[2] == "DELETE", "DELETE should be allowed");
                    foundMethods = true;
                }
                else if (it->first == "index") {
                    ASSERT(it->second[0] == "index.html", "Index should be index.html");
                    foundIndex = true;
                }
            }
            
            ASSERT(foundMethods, "Allowed methods directive not found");
            ASSERT(foundIndex, "Index directive not found");
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Unexpected exception: " << e.what() << std::endl;
            return false;
        }
    }

    bool testInvalidPort() {
        try {
            parseConfig("config_tests/test_configs/invalid_port.conf");
            std::cerr << "Should have thrown exception for invalid port" << std::endl;
            return false;
        } catch (const std::exception& e) {
            return true;
        }
    }

    bool testMissingBrackets() {
        try {
            parseConfig("config_tests/test_configs/missing_brackets.conf");
            std::cerr << "Should have thrown exception for missing brackets" << std::endl;
            return false;
        } catch (const std::exception& e) {
            return true;
        }
    }

    bool testErrorConfig() {
        std::cout << "\nTesting error.conf for various error cases..." << std::endl;
        
        try {
            Config config = parseConfig("config_tests/test_configs/error.conf");
            std::cerr << "No error thrown when parsing invalid config" << std::endl;
            return false;
        } catch (const std::exception& e) {
            string error = e.what();
            if (error.find("Missing arguments of the \"server_name\"") != string::npos) {
                return true;
            }
            std::cerr << "No expected error was caught. Got: " << e.what() << std::endl;
            return false;
        }
    }

    bool testQuotedStrings() {
        std::cout << "\nTesting quoted strings..." << std::endl;
        
        try {
            Config config = parseConfig("config_tests/test_configs/quoted_strings.conf");
            
            // Test server name with spaces
            bool hasComplexServerName = false;
            bool hasSpacedRoot = false;
            bool hasSpecialChars = false;
            bool hasMultiline = false;
            bool hasEmptyString = false;

            // Check server directives
            for (Directives::iterator it = config.second[0].first.begin(); 
                 it != config.second[0].first.end(); ++it) {
                if (it->first == "server_name" && 
                    it->second[0] == "my complex server name") {
                    hasComplexServerName = true;
                }
                else if (it->first == "root" && 
                         it->second[0] == "/path/with/spaces/www/root") {
                    hasSpacedRoot = true;
                }
            }

            // Check location blocks
            for (LocationContexts::iterator it = config.second[0].second.begin(); 
                 it != config.second[0].second.end(); ++it) {
                if (it->first == "/api/v1/test") {
                    for (Directives::iterator dit = it->second.begin(); 
                         dit != it->second.end(); ++dit) {
                        if (dit->first == "index" && 
                            dit->second[0] == "special@index.html") {
                            hasSpecialChars = true;
                        }
                    }
                }
                else if (it->first == "/multi/line") {
                    // Check for multiline string
                    for (Directives::iterator dit = it->second.begin(); 
                         dit != it->second.end(); ++dit) {
                        if (dit->first == "index" && 
                            dit->second[0] == "This is a multiline string in the config") {
                            hasMultiline = true;
                        }
                    }
                }
                else if (it->first == "") {
                    hasEmptyString = true;
                }
            }

            if (!hasComplexServerName) {
                std::cerr << "Failed to parse server name with spaces" << std::endl;
                return false;
            }
            if (!hasSpacedRoot) {
                std::cerr << "Failed to parse root path with spaces" << std::endl;
                return false;
            }
            if (!hasSpecialChars) {
                std::cerr << "Failed to parse location with special characters" << std::endl;
                return false;
            }
            if (!hasMultiline) {
                std::cerr << "Failed to parse multiline string" << std::endl;
                return false;
            }
            if (!hasEmptyString) {
                std::cerr << "Failed to parse empty string" << std::endl;
                return false;
            }

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Unexpected exception: " << e.what() << std::endl;
            return false;
        }
    }

public:
    bool runAllTests() {
        std::cout << "\nRunning Config Parser Tests...\n" << std::endl;
        
        std::cout << "1. Testing valid simple config..." << std::endl;
        if (!testValidSimpleConfig()) {
            std::cerr << "❌ Valid simple config test failed" << std::endl;
            return false;
        }
        std::cout << "✅ Valid simple config test passed" << std::endl;

        std::cout << "\n2. Testing invalid port..." << std::endl;
        if (!testInvalidPort()) {
            std::cerr << "❌ Invalid port test failed" << std::endl;
            return false;
        }
        std::cout << "✅ Invalid port test passed" << std::endl;

        std::cout << "\n3. Testing missing brackets..." << std::endl;
        if (!testMissingBrackets()) {
            std::cerr << "❌ Missing brackets test failed" << std::endl;
            return false;
        }
        std::cout << "✅ Missing brackets test passed" << std::endl;

        std::cout << "\n4. Testing error config..." << std::endl;
        if (!testErrorConfig()) {
            std::cerr << "❌ Error config test failed" << std::endl;
            return false;
        }
        std::cout << "✅ Error config test passed" << std::endl;

        std::cout << "\n5. Testing quoted strings..." << std::endl;
        if (!testQuotedStrings()) {
            std::cerr << "❌ Quoted strings test failed" << std::endl;
            return false;
        }
        std::cout << "✅ Quoted strings test passed" << std::endl;

        std::cout << "\nAll config parser tests passed! ✅\n" << std::endl;
        return true;
    }
};

int main() {
    ConfigParserTest tester;
    return tester.runAllTests() ? 0 : 1;
}
