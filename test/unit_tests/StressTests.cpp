#include "catch.hpp"
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

// Helper function to execute a command and get its output
std::pair<int, std::string> executeCommand(const std::string& command) {
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return std::make_pair(-1, "Error executing command");
    }

    char buffer[128];
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL) {
            result += buffer;
        }
    }

    int status = pclose(pipe);
    return std::make_pair(WEXITSTATUS(status), result);
}

// Helper function to check if a process is running
bool isProcessRunning(pid_t pid) {
    return kill(pid, 0) == 0;
}

// Helper function to get memory usage of a process (in KB)
int getMemoryUsage(pid_t pid) {
    std::string command = "ps -o rss= -p " + std::to_string(pid);
    std::pair<int, std::string> result = executeCommand(command);
    if (result.first != 0 || result.second.empty()) {
        return -1;
    }

    return std::atoi(result.second.c_str());
}

// Helper function to check if siege is installed
bool isSiegeInstalled() {
    std::pair<int, std::string> result = executeCommand("which siege");
    return result.first == 0;
}

// Helper function to create a temporary configuration file
static std::string createTempConfigFile(const std::string& content) {
    std::string tempFilePath = "test_stress_config.conf";
    std::ofstream tempFile(tempFilePath.c_str());
    tempFile << content;
    tempFile.close();
    return tempFilePath;
}

// Helper function to delete a temporary file
static void deleteTempFile(const std::string& path) {
    unlink(path.c_str());
}

// Helper function to create a simple HTML file
std::string createSimpleHtmlFile(const std::string& content, const std::string& path) {
    std::ofstream htmlFile(path.c_str());
    htmlFile << content;
    htmlFile.close();
    return path;
}

// Mock server class for stress testing
class StressTestServer {
private:
    pid_t pid;
    std::string configPath;
    std::string rootDir;
    int port;

public:
    StressTestServer(const std::string& configPath, const std::string& rootDir, int port)
        : pid(-1), configPath(configPath), rootDir(rootDir), port(port) {}

    ~StressTestServer() {
        stop();
    }

    bool start() {
        // Create the root directory if it doesn't exist
        std::string command = "mkdir -p " + rootDir;
        system(command.c_str());

        // Fork a child process to run the server
        pid = fork();
        if (pid == -1) {
            // Fork failed
            return false;
        } else if (pid == 0) {
            // Child process
            // Redirect stdout and stderr to /dev/null
            freopen("/dev/null", "w", stdout);
            freopen("/dev/null", "w", stderr);

            // Execute the server
            execl("./webserv", "webserv", "-c", configPath.c_str(), NULL);

            // If execl returns, it failed
            exit(1);
        }

        // Parent process
        // Wait a moment for the server to start
        sleep(2);

        // Check if the server is running
        return isProcessRunning(pid);
    }

    void stop() {
        if (pid > 0) {
            // Send SIGTERM to the server
            kill(pid, SIGTERM);

            // Wait for the server to exit
            int status;
            waitpid(pid, &status, 0);

            pid = -1;
        }
    }

    bool isRunning() const {
        return pid > 0 && isProcessRunning(pid);
    }

    pid_t getPid() const {
        return pid;
    }

    int getPort() const {
        return port;
    }

    std::string getRootDir() const {
        return rootDir;
    }
};

TEST_CASE("Stress Tests", "[.][stress]") {
    // Skip these tests if siege is not installed
    if (!isSiegeInstalled()) {
        WARN("Skipping stress tests because siege is not installed");
        return;
    }

    // Create a temporary directory for the stress tests
    std::string testDir = "test_stress_dir";
    std::string command = "mkdir -p " + testDir;
    system(command.c_str());

    // Create a simple HTML file
    std::string htmlContent = "<!DOCTYPE html><html><head><title>Test</title></head><body>Test</body></html>";
    std::string htmlPath = testDir + "/index.html";
    createSimpleHtmlFile(htmlContent, htmlPath);

    // Create a configuration file
    std::string configContent =
        "server {\n"
        "    listen 127.0.0.1:8085;\n"
        "    server_name localhost;\n"
        "    root ./" + testDir + ";\n"
        "    index index.html;\n"
        "}\n";
    std::string configPath = createTempConfigFile(configContent);

    // Create a server instance
    StressTestServer server(configPath, testDir, 8085);

    SECTION("Test Server Start and Stop") {
        // Start the server
        bool started = server.start();
        REQUIRE(started);

        // Check that the server is running
        REQUIRE(server.isRunning());

        // Stop the server
        server.stop();

        // Check that the server is not running
        REQUIRE_FALSE(server.isRunning());
    }

    SECTION("Test Availability") {
        // Start the server
        bool started = server.start();
        REQUIRE(started);

        // Run siege for a short time
        std::string siegeCommand = "siege -b -c 10 -t5S http://localhost:8085/ 2>&1";
        std::pair<int, std::string> siegeResult = executeCommand(siegeCommand);

        // Extract availability from siege output
        std::string availabilityStr;
        std::istringstream iss(siegeResult.second);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.find("Availability") != std::string::npos) {
                size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    availabilityStr = line.substr(pos + 1);
                    // Trim whitespace and remove %
                    availabilityStr.erase(0, availabilityStr.find_first_not_of(" \t"));
                    availabilityStr.erase(availabilityStr.find_last_not_of("% \t") + 1);
                    break;
                }
            }
        }

        // Convert availability to float
        float availability = std::atof(availabilityStr.c_str());

        // Check that availability is above 99.5%
        REQUIRE(availability >= 99.5f);

        // Stop the server
        server.stop();
    }

    SECTION("Test Memory Leak") {
        // Start the server
        bool started = server.start();
        REQUIRE(started);

        // Get initial memory usage
        int initialMemory = getMemoryUsage(server.getPid());
        REQUIRE(initialMemory > 0);

        // Run siege for a short time
        std::string siegeCommand = "siege -b -c 50 -t10S http://localhost:8085/ > /dev/null 2>&1";
        system(siegeCommand.c_str());

        // Get final memory usage
        int finalMemory = getMemoryUsage(server.getPid());
        REQUIRE(finalMemory > 0);

        // Calculate memory growth
        int memoryGrowth = finalMemory - initialMemory;

        // Check that memory growth is not excessive
        // Allow for some growth, but not more than 50% or 10MB
        REQUIRE(memoryGrowth < initialMemory * 0.5);
        REQUIRE(memoryGrowth < 10240);

        // Stop the server
        server.stop();
    }

    SECTION("Test Hanging Connections") {
        // Start the server
        bool started = server.start();
        REQUIRE(started);

        // Run siege for a short time
        std::string siegeCommand = "siege -b -c 50 -t5S http://localhost:8085/ > /dev/null 2>&1";
        system(siegeCommand.c_str());

        // Wait a moment for connections to close
        sleep(2);

        // Check for established connections
        std::string netstatCommand = "netstat -an | grep 8085 | grep ESTABLISHED | wc -l";
        std::pair<int, std::string> netstatResult = executeCommand(netstatCommand);

        // Convert result to int
        int connections = std::atoi(netstatResult.second.c_str());

        // Check that there are no hanging connections
        REQUIRE(connections == 0);

        // Stop the server
        server.stop();
    }

    SECTION("Test Long-term Stability") {
        // Start the server
        bool started = server.start();
        REQUIRE(started);

        // Run siege for a longer time
        std::string siegeCommand = "siege -b -c 50 -t30S http://localhost:8085/ 2>&1";
        std::pair<int, std::string> siegeResult = executeCommand(siegeCommand);

        // Extract availability from siege output
        std::string availabilityStr;
        std::istringstream iss(siegeResult.second);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.find("Availability") != std::string::npos) {
                size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    availabilityStr = line.substr(pos + 1);
                    // Trim whitespace and remove %
                    availabilityStr.erase(0, availabilityStr.find_first_not_of(" \t"));
                    availabilityStr.erase(availabilityStr.find_last_not_of("% \t") + 1);
                    break;
                }
            }
        }

        // Convert availability to float
        float availability = std::atof(availabilityStr.c_str());

        // Check that availability is above 99.5%
        REQUIRE(availability >= 99.5f);

        // Stop the server
        server.stop();
    }

    // Clean up
    deleteTempFile(configPath);
    command = "rm -rf " + testDir;
    system(command.c_str());
}
