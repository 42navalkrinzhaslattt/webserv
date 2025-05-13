#include "catch.hpp"
#include "HttpServer.hpp"
#include "Config.hpp"
#include "Logger.hpp"
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

// Forward declarations for functions we need to test
extern bool directiveExists(const std::vector<std::vector<std::string> >& directives, const std::string& name);
extern std::vector<std::string> getFirstDirective(const std::vector<std::vector<std::string> >& directives, const std::string& name);
extern std::vector<std::vector<std::string> > getAllDirectives(const std::vector<std::vector<std::string> >& directives, const std::string& name);

// Helper function to create a temporary config file
std::string createTempConfigFile(const std::string& content) {
    std::string tempFilePath = "test_config.conf";
    std::ofstream tempFile(tempFilePath.c_str());
    tempFile << content;
    tempFile.close();
    return tempFilePath;
}

// Helper function to delete a temporary config file
void deleteTempConfigFile(const std::string& path) {
    std::remove(path.c_str());
}

TEST_CASE("Configuration Parsing Tests") {
    std::stringstream logStream;
    Logger log(logStream, "DEBUG");

    SECTION("Test Multiple Servers with Different Ports") {
        std::string configContent =
            "server {\n"
            "    listen 127.0.0.1:8080;\n"
            "    server_name localhost;\n"
            "    root ./html/default;\n"
            "}\n"
            "server {\n"
            "    listen 127.0.0.1:8081;\n"
            "    server_name localhost;\n"
            "    root ./html/test;\n"
            "}\n";

        std::string tempFilePath = createTempConfigFile(configContent);

        // Only check config, don't start server
        HttpServer server(tempFilePath, log, true);

        // Check that we have 2 servers
        REQUIRE(server.getServerConfigs().size() == 2);

        // Check that the ports are correct
        REQUIRE(server.getServerConfigs()[0].port == 8080);
        REQUIRE(server.getServerConfigs()[1].port == 8081);

        deleteTempConfigFile(tempFilePath);
    }

    SECTION("Test Multiple Servers with Different Hostnames") {
        std::string configContent =
            "server {\n"
            "    listen 127.0.0.1:8080;\n"
            "    server_name localhost;\n"
            "    root ./html/default;\n"
            "}\n"
            "server {\n"
            "    listen 127.0.0.1:8080;\n"
            "    server_name example.com;\n"
            "    root ./html/test;\n"
            "}\n";

        std::string tempFilePath = createTempConfigFile(configContent);

        // Only check config, don't start server
        HttpServer server(tempFilePath, log, true);

        // Check that we have 2 servers
        REQUIRE(server.getServerConfigs().size() == 2);

        // Check that the hostnames are correct
        REQUIRE(server.getServerConfigs()[0].serverName == "localhost");
        REQUIRE(server.getServerConfigs()[1].serverName == "example.com");

        deleteTempConfigFile(tempFilePath);
    }

    SECTION("Test Default Error Pages") {
        std::string configContent =
            "server {\n"
            "    listen 127.0.0.1:8080;\n"
            "    server_name localhost;\n"
            "    root ./html/default;\n"
            "    error_page 404 /error/404.html;\n"
            "    error_page 500 502 503 504 /error/50x.html;\n"
            "}\n";

        std::string tempFilePath = createTempConfigFile(configContent);

        // Only check config, don't start server
        HttpServer server(tempFilePath, log, true);

        // Check that we have 1 server
        REQUIRE(server.getServerConfigs().size() == 1);

        // Check that the error pages are correctly parsed
        const ServerConfig& config = server.getServerConfigs()[0];
        const LocationCtx& defaultLocation = config.defaultLocation;

        // Check if error_page directive exists
        REQUIRE(directiveExists(defaultLocation.second, "error_page"));

        // Get all error_page directives
        ArgResults errorPages = getAllDirectives(defaultLocation.second, "error_page");

        // Check that we have 2 error_page directives
        REQUIRE(errorPages.size() == 2);

        // Check the first error_page directive
        REQUIRE(errorPages[0].size() == 2);
        REQUIRE(errorPages[0][0] == "404");
        REQUIRE(errorPages[0][1] == "/error/404.html");

        // Check the second error_page directive
        REQUIRE(errorPages[1].size() == 5);
        REQUIRE(errorPages[1][0] == "500");
        REQUIRE(errorPages[1][1] == "502");
        REQUIRE(errorPages[1][2] == "503");
        REQUIRE(errorPages[1][3] == "504");
        REQUIRE(errorPages[1][4] == "/error/50x.html");

        deleteTempConfigFile(tempFilePath);
    }

    SECTION("Test Client Body Size Limit") {
        std::string configContent =
            "server {\n"
            "    listen 127.0.0.1:8080;\n"
            "    server_name localhost;\n"
            "    root ./html/default;\n"
            "    client_max_body_size 10m;\n"
            "    location /upload {\n"
            "        client_max_body_size 500;\n"
            "    }\n"
            "}\n";

        std::string tempFilePath = createTempConfigFile(configContent);

        // Only check config, don't start server
        HttpServer server(tempFilePath, log, true);

        // Check that we have 1 server
        REQUIRE(server.getServerConfigs().size() == 1);

        // Check that the client_max_body_size is correctly parsed
        const ServerConfig& config = server.getServerConfigs()[0];
        const LocationCtx& defaultLocation = config.defaultLocation;

        // Check if client_max_body_size directive exists in default location
        REQUIRE(directiveExists(defaultLocation.second, "client_max_body_size"));

        // Get client_max_body_size directive from default location
        std::string defaultMaxBodySize = getFirstDirective(defaultLocation.second, "client_max_body_size")[1];
        REQUIRE(defaultMaxBodySize == "10m");

        // Check if /upload location exists
        bool uploadLocationExists = false;
        LocationCtx uploadLocation;

        for (const LocationCtx& location : config.locations) {
            if (location.first == "/upload") {
                uploadLocationExists = true;
                uploadLocation = location;
                break;
            }
        }

        REQUIRE(uploadLocationExists);

        // Check if client_max_body_size directive exists in /upload location
        REQUIRE(directiveExists(uploadLocation.second, "client_max_body_size"));

        // Get client_max_body_size directive from /upload location
        std::string uploadMaxBodySize = getFirstDirective(uploadLocation.second, "client_max_body_size")[1];
        REQUIRE(uploadMaxBodySize == "500");

        deleteTempConfigFile(tempFilePath);
    }

    SECTION("Test Routes in a Server to Different Directories") {
        std::string configContent =
            "server {\n"
            "    listen 127.0.0.1:8080;\n"
            "    server_name localhost;\n"
            "    root ./html/default;\n"
            "    location /test {\n"
            "        root ./html/test;\n"
            "    }\n"
            "    location /static {\n"
            "        root ./html/static;\n"
            "    }\n"
            "}\n";

        std::string tempFilePath = createTempConfigFile(configContent);

        // Only check config, don't start server
        HttpServer server(tempFilePath, log, true);

        // Check that we have 1 server
        REQUIRE(server.getServerConfigs().size() == 1);

        // Check that the locations are correctly parsed
        const ServerConfig& config = server.getServerConfigs()[0];

        // Check if /test location exists
        bool testLocationExists = false;
        LocationCtx testLocation;

        // Check if /static location exists
        bool staticLocationExists = false;
        LocationCtx staticLocation;

        for (const LocationCtx& location : config.locations) {
            if (location.first == "/test") {
                testLocationExists = true;
                testLocation = location;
            } else if (location.first == "/static") {
                staticLocationExists = true;
                staticLocation = location;
            }
        }

        REQUIRE(testLocationExists);
        REQUIRE(staticLocationExists);

        // Check if root directive exists in /test location
        REQUIRE(directiveExists(testLocation.second, "root"));

        // Get root directive from /test location
        std::string testRoot = getFirstDirective(testLocation.second, "root")[1];
        REQUIRE(testRoot == "./html/test");

        // Check if root directive exists in /static location
        REQUIRE(directiveExists(staticLocation.second, "root"));

        // Get root directive from /static location
        std::string staticRoot = getFirstDirective(staticLocation.second, "root")[1];
        REQUIRE(staticRoot == "./html/static");

        deleteTempConfigFile(tempFilePath);
    }

    SECTION("Test Default File to Search for in a Directory") {
        std::string configContent =
            "server {\n"
            "    listen 127.0.0.1:8080;\n"
            "    server_name localhost;\n"
            "    root ./html/default;\n"
            "    index index.html index.htm welcome.html;\n"
            "    location /test {\n"
            "        index test.html;\n"
            "    }\n"
            "}\n";

        std::string tempFilePath = createTempConfigFile(configContent);

        // Only check config, don't start server
        HttpServer server(tempFilePath, log, true);

        // Check that we have 1 server
        REQUIRE(server.getServerConfigs().size() == 1);

        // Check that the index files are correctly parsed
        const ServerConfig& config = server.getServerConfigs()[0];
        const LocationCtx& defaultLocation = config.defaultLocation;

        // Check if index directive exists in default location
        REQUIRE(directiveExists(defaultLocation.second, "index"));

        // Get index directive from default location
        ArgResults defaultIndex = getFirstDirective(defaultLocation.second, "index");
        REQUIRE(defaultIndex.size() == 3);
        REQUIRE(defaultIndex[0] == "index.html");
        REQUIRE(defaultIndex[1] == "index.htm");
        REQUIRE(defaultIndex[2] == "welcome.html");

        // Check if /test location exists
        bool testLocationExists = false;
        LocationCtx testLocation;

        for (const LocationCtx& location : config.locations) {
            if (location.first == "/test") {
                testLocationExists = true;
                testLocation = location;
                break;
            }
        }

        REQUIRE(testLocationExists);

        // Check if index directive exists in /test location
        REQUIRE(directiveExists(testLocation.second, "index"));

        // Get index directive from /test location
        ArgResults testIndex = getFirstDirective(testLocation.second, "index");
        REQUIRE(testIndex.size() == 1);
        REQUIRE(testIndex[0] == "test.html");

        deleteTempConfigFile(tempFilePath);
    }

    SECTION("Test Methods Accepted for a Certain Route") {
        std::string configContent =
            "server {\n"
            "    listen 127.0.0.1:8080;\n"
            "    server_name localhost;\n"
            "    root ./html/default;\n"
            "    location / {\n"
            "        limit_except GET POST DELETE;\n"
            "    }\n"
            "    location /test {\n"
            "        limit_except GET;\n"
            "    }\n"
            "    location /upload {\n"
            "        limit_except POST;\n"
            "    }\n"
            "}\n";

        std::string tempFilePath = createTempConfigFile(configContent);

        // Only check config, don't start server
        HttpServer server(tempFilePath, log, true);

        // Check that we have 1 server
        REQUIRE(server.getServerConfigs().size() == 1);

        // Check that the limit_except directives are correctly parsed
        const ServerConfig& config = server.getServerConfigs()[0];

        // Check if / location exists
        bool rootLocationExists = false;
        LocationCtx rootLocation;

        // Check if /test location exists
        bool testLocationExists = false;
        LocationCtx testLocation;

        // Check if /upload location exists
        bool uploadLocationExists = false;
        LocationCtx uploadLocation;

        for (const LocationCtx& location : config.locations) {
            if (location.first == "/") {
                rootLocationExists = true;
                rootLocation = location;
            } else if (location.first == "/test") {
                testLocationExists = true;
                testLocation = location;
            } else if (location.first == "/upload") {
                uploadLocationExists = true;
                uploadLocation = location;
            }
        }

        REQUIRE(rootLocationExists);
        REQUIRE(testLocationExists);
        REQUIRE(uploadLocationExists);

        // Check if limit_except directive exists in / location
        REQUIRE(directiveExists(rootLocation.second, "limit_except"));

        // Get limit_except directive from / location
        ArgResults rootLimitExcept = getFirstDirective(rootLocation.second, "limit_except");
        REQUIRE(rootLimitExcept.size() == 3);
        REQUIRE(rootLimitExcept[0] == "GET");
        REQUIRE(rootLimitExcept[1] == "POST");
        REQUIRE(rootLimitExcept[2] == "DELETE");

        // Check if limit_except directive exists in /test location
        REQUIRE(directiveExists(testLocation.second, "limit_except"));

        // Get limit_except directive from /test location
        ArgResults testLimitExcept = getFirstDirective(testLocation.second, "limit_except");
        REQUIRE(testLimitExcept.size() == 1);
        REQUIRE(testLimitExcept[0] == "GET");

        // Check if limit_except directive exists in /upload location
        REQUIRE(directiveExists(uploadLocation.second, "limit_except"));

        // Get limit_except directive from /upload location
        ArgResults uploadLimitExcept = getFirstDirective(uploadLocation.second, "limit_except");
        REQUIRE(uploadLimitExcept.size() == 1);
        REQUIRE(uploadLimitExcept[0] == "POST");

        deleteTempConfigFile(tempFilePath);
    }
}
