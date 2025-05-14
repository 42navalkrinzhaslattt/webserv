#include "catch.hpp"
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Helper function to create a temporary configuration file
std::string createTempConfigFile(const std::string& content) {
    std::string tempFilePath = "test_config.conf";
    std::ofstream tempFile(tempFilePath.c_str());
    tempFile << content;
    tempFile.close();
    return tempFilePath;
}

// Helper function to delete a temporary file
void deleteTempFile(const std::string& path) {
    unlink(path.c_str());
}

// Helper function to check if a port is in use
bool isPortInUse(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return true; // Error creating socket, assume port is in use
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int result = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    close(sock);

    return result < 0; // If bind fails, port is in use
}

// Mock server configuration class
class ServerConfig {
public:
    std::string host;
    int port;
    std::string serverName;
    std::string root;
    std::map<std::string, std::string> locations;

    ServerConfig(const std::string& host, int port, const std::string& serverName, const std::string& root)
        : host(host), port(port), serverName(serverName), root(root) {}

    void addLocation(const std::string& path, const std::string& root) {
        locations[path] = root;
    }
};

// Mock server class
class MockServer {
private:
    std::vector<ServerConfig> configs;
    std::vector<int> activePorts;
    bool running;

public:
    MockServer() : running(false) {}

    ~MockServer() {
        stop();
    }

    void addConfig(const ServerConfig& config) {
        configs.push_back(config);
    }

    bool start() {
        if (running) {
            return false;
        }

        // Check for duplicate ports
        std::map<int, bool> portMap;
        for (size_t i = 0; i < configs.size(); i++) {
            int port = configs[i].port;
            if (portMap.find(port) != portMap.end()) {
                // Duplicate port found
                return false;
            }
            portMap[port] = true;

            // Check if port is already in use
            if (isPortInUse(port)) {
                // Port is already in use
                return false;
            }

            // Simulate binding to the port
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                // Failed to create socket
                return false;
            }

            // Set socket options to allow reuse
            int opt = 1;
            if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
                close(sock);
                return false;
            }

            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = inet_addr(configs[i].host.c_str());

            if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                // Failed to bind to port
                close(sock);
                return false;
            }

            if (listen(sock, 10) < 0) {
                // Failed to listen on port
                close(sock);
                return false;
            }

            activePorts.push_back(sock);
        }

        running = true;
        return true;
    }

    void stop() {
        if (!running) {
            return;
        }

        // Close all active ports
        for (size_t i = 0; i < activePorts.size(); i++) {
            close(activePorts[i]);
        }
        activePorts.clear();

        running = false;
    }

    bool isRunning() const {
        return running;
    }

    size_t getConfigCount() const {
        return configs.size();
    }

    std::vector<int> getActivePorts() const {
        return activePorts;
    }
};

// Mock configuration parser
class ConfigParser {
public:
    static std::vector<ServerConfig> parse(const std::string& configPath) {
        std::vector<ServerConfig> configs;
        std::ifstream configFile(configPath.c_str());
        std::string line;
        ServerConfig* currentConfig = NULL;

        while (std::getline(configFile, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }

            // Trim leading and trailing whitespace
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);

            if (line.find("server {") != std::string::npos) {
                // Start of a new server block
                currentConfig = NULL;
            } else if (line.find("}") != std::string::npos) {
                // End of a server block
                currentConfig = NULL;
            } else if (line.find("listen") != std::string::npos) {
                // Parse listen directive
                size_t pos = line.find("listen") + 6;
                line.erase(0, pos);
                line.erase(0, line.find_first_not_of(" \t"));
                line.erase(line.find_last_not_of(";") + 1);

                std::string host = "0.0.0.0";
                int port = 80;

                if (line.find(":") != std::string::npos) {
                    // Format: host:port
                    host = line.substr(0, line.find(":"));
                    port = atoi(line.substr(line.find(":") + 1).c_str());
                } else {
                    // Format: port only
                    port = atoi(line.c_str());
                }

                if (currentConfig == NULL) {
                    configs.push_back(ServerConfig(host, port, "", ""));
                    currentConfig = &configs.back();
                } else {
                    currentConfig->host = host;
                    currentConfig->port = port;
                }
            } else if (line.find("server_name") != std::string::npos) {
                // Parse server_name directive
                size_t pos = line.find("server_name") + 11;
                line.erase(0, pos);
                line.erase(0, line.find_first_not_of(" \t"));
                line.erase(line.find_last_not_of(";") + 1);

                if (currentConfig != NULL) {
                    currentConfig->serverName = line;
                }
            } else if (line.find("root") != std::string::npos) {
                // Parse root directive
                size_t pos = line.find("root") + 4;
                line.erase(0, pos);
                line.erase(0, line.find_first_not_of(" \t"));
                line.erase(line.find_last_not_of(";") + 1);

                if (currentConfig != NULL) {
                    currentConfig->root = line;
                }
            } else if (line.find("location") != std::string::npos) {
                // Parse location directive
                size_t pos = line.find("location") + 8;
                line.erase(0, pos);
                line.erase(0, line.find_first_not_of(" \t"));

                std::string path = line.substr(0, line.find("{"));
                path.erase(0, path.find_first_not_of(" \t"));
                path.erase(path.find_last_not_of(" \t") + 1);

                // For simplicity, we'll just set a default root for the location
                if (currentConfig != NULL) {
                    currentConfig->addLocation(path, currentConfig->root + path);
                }
            }
        }

        return configs;
    }
};

TEST_CASE("Port Configuration Tests") {
    SECTION("Test Multiple Ports with Different Websites") {
        // Create a configuration with multiple ports
        std::string configContent =
            "server {\n"
            "    listen 127.0.0.1:8091;\n"
            "    server_name site1.local;\n"
            "    root ./www/site1;\n"
            "}\n"
            "\n"
            "server {\n"
            "    listen 127.0.0.1:8092;\n"
            "    server_name site2.local;\n"
            "    root ./www/site2;\n"
            "}\n";

        std::string configPath = createTempConfigFile(configContent);

        // Parse the configuration
        std::vector<ServerConfig> configs = ConfigParser::parse(configPath);

        // Check that we have two server configurations
        REQUIRE(configs.size() == 2);

        // Check the first server configuration
        REQUIRE(configs[0].host == "127.0.0.1");
        REQUIRE(configs[0].port == 8091);
        REQUIRE(configs[0].serverName == "site1.local");
        REQUIRE(configs[0].root == "./www/site1");

        // Check the second server configuration
        REQUIRE(configs[1].host == "127.0.0.1");
        REQUIRE(configs[1].port == 8092);
        REQUIRE(configs[1].serverName == "site2.local");
        REQUIRE(configs[1].root == "./www/site2");

        // Create a mock server and add the configurations
        MockServer server;
        for (size_t i = 0; i < configs.size(); i++) {
            server.addConfig(configs[i]);
        }

        // Start the server
        bool started = server.start();
        REQUIRE(started);

        // Check that the server is running
        REQUIRE(server.isRunning());

        // Check that we have two active ports
        REQUIRE(server.getActivePorts().size() == 2);

        // Stop the server
        server.stop();

        // Check that the server is not running
        REQUIRE_FALSE(server.isRunning());

        // Clean up
        deleteTempFile(configPath);
    }

    SECTION("Test Duplicate Port Configuration") {
        // Create a configuration with duplicate ports
        std::string configContent =
            "server {\n"
            "    listen 127.0.0.1:8093;\n"
            "    server_name site1.local;\n"
            "    root ./www/site1;\n"
            "}\n"
            "\n"
            "server {\n"
            "    listen 127.0.0.1:8093;\n"
            "    server_name site2.local;\n"
            "    root ./www/site2;\n"
            "}\n";

        std::string configPath = createTempConfigFile(configContent);

        // Parse the configuration
        std::vector<ServerConfig> configs = ConfigParser::parse(configPath);

        // Check that we have two server configurations
        REQUIRE(configs.size() == 2);

        // Check that both configurations have the same port
        REQUIRE(configs[0].port == configs[1].port);

        // Create a mock server and add the configurations
        MockServer server;
        for (size_t i = 0; i < configs.size(); i++) {
            server.addConfig(configs[i]);
        }

        // Try to start the server
        bool started = server.start();

        // The server should not start due to duplicate ports
        REQUIRE_FALSE(started);

        // Check that the server is not running
        REQUIRE_FALSE(server.isRunning());

        // Clean up
        deleteTempFile(configPath);
    }

    SECTION("Test Multiple Servers with Overlapping Ports") {
        // Create configurations for two servers with overlapping ports
        std::string configContent1 =
            "server {\n"
            "    listen 127.0.0.1:8090;\n"
            "    server_name site1.local;\n"
            "    root ./www/site1;\n"
            "}\n";

        std::string configContent2 =
            "server {\n"
            "    listen 127.0.0.1:8090;\n"
            "    server_name site2.local;\n"
            "    root ./www/site2;\n"
            "}\n";

        std::string configPath1 = createTempConfigFile(configContent1);
        std::string configPath2 = "test_config2.conf";
        std::ofstream configFile2(configPath2.c_str());
        configFile2 << configContent2;
        configFile2.close();

        // Parse the configurations
        std::vector<ServerConfig> configs1 = ConfigParser::parse(configPath1);
        std::vector<ServerConfig> configs2 = ConfigParser::parse(configPath2);

        // Check that we have one server configuration in each file
        REQUIRE(configs1.size() == 1);
        REQUIRE(configs2.size() == 1);

        // Check that both configurations have the same port
        REQUIRE(configs1[0].port == configs2[0].port);

        // Create two mock servers
        MockServer server1;
        MockServer server2;

        // Add the configurations to the servers
        server1.addConfig(configs1[0]);
        server2.addConfig(configs2[0]);

        // Start the first server
        bool started1 = server1.start();

        // If the first server started successfully, test the second server
        if (started1) {
            // Try to start the second server
            bool started2 = server2.start();

            // The second server should not start due to the port being in use
            REQUIRE_FALSE(started2);

            // Check that only the first server is running
            REQUIRE(server1.isRunning());
            REQUIRE_FALSE(server2.isRunning());

            // Stop the first server
            server1.stop();

            // Check that the first server is not running
            REQUIRE_FALSE(server1.isRunning());

            // Try to start the second server again
            started2 = server2.start();

            // Now the second server should start
            REQUIRE(started2);

            // Check that the second server is running
            REQUIRE(server2.isRunning());

            // Stop the second server
            server2.stop();

            // Check that the second server is not running
            REQUIRE_FALSE(server2.isRunning());
        } else {
            // If the first server failed to start, it might be because the port is already in use
            // In this case, we'll skip the test
            WARN("Skipping test because port 8090 is already in use");
        }

        // Clean up
        deleteTempFile(configPath1);
        deleteTempFile(configPath2);
    }
}
