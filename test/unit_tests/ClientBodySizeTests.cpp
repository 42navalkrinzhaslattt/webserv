#include "catch.hpp"
#include "HttpServer.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <sstream>
#include <string>

// Helper class to expose private methods for testing
class TestHttpServer : public HttpServer {
public:
    TestHttpServer(const std::string &configPath, Logger &log, bool onlyCheckConfig)
        : HttpServer(configPath, log, onlyCheckConfig) {}
    
    // Expose the checkRequestBodySize method
    bool testCheckRequestBodySize(const HttpRequest &request, size_t bodySize) {
        return checkRequestBodySize(0, request, bodySize); // Use a dummy client socket
    }
};

TEST_CASE("Client Body Size Limit Tests") {
    std::stringstream logStream;
    Logger log(logStream, "DEBUG");
    
    // Create a test config file
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
    
    std::string tempFilePath = "test_body_size.conf";
    std::ofstream tempFile(tempFilePath.c_str());
    tempFile << configContent;
    tempFile.close();
    
    // Create a test server
    TestHttpServer server(tempFilePath, log, true);
    
    SECTION("Test Utils::convertSizeToBytes") {
        // Test with bytes (no suffix)
        REQUIRE(Utils::convertSizeToBytes("1024") == 1024);
        
        // Test with kilobytes (k or K suffix)
        REQUIRE(Utils::convertSizeToBytes("1k") == 1024);
        REQUIRE(Utils::convertSizeToBytes("1K") == 1024);
        
        // Test with megabytes (m or M suffix)
        REQUIRE(Utils::convertSizeToBytes("1m") == 1024 * 1024);
        REQUIRE(Utils::convertSizeToBytes("1M") == 1024 * 1024);
        
        // Test with gigabytes (g or G suffix)
        REQUIRE(Utils::convertSizeToBytes("1g") == 1024 * 1024 * 1024);
        REQUIRE(Utils::convertSizeToBytes("1G") == 1024 * 1024 * 1024);
    }
    
    SECTION("Test Default Location Body Size Limit") {
        // Create a request for the root location
        HttpRequest request;
        request.method = "POST";
        request.path = "/";
        
        // Test with a small body (should be allowed)
        REQUIRE(server.testCheckRequestBodySize(request, 1024) == true);
        
        // Test with a large body (should be allowed)
        REQUIRE(server.testCheckRequestBodySize(request, 5 * 1024 * 1024) == true);
        
        // Test with a body larger than the limit (should be rejected)
        REQUIRE(server.testCheckRequestBodySize(request, 11 * 1024 * 1024) == false);
    }
    
    SECTION("Test Upload Location Body Size Limit") {
        // Create a request for the /upload location
        HttpRequest request;
        request.method = "POST";
        request.path = "/upload";
        
        // Test with a small body (should be allowed)
        REQUIRE(server.testCheckRequestBodySize(request, 100) == true);
        
        // Test with a body at the limit (should be allowed)
        REQUIRE(server.testCheckRequestBodySize(request, 500) == true);
        
        // Test with a body larger than the limit (should be rejected)
        REQUIRE(server.testCheckRequestBodySize(request, 501) == false);
    }
    
    // Clean up
    std::remove(tempFilePath.c_str());
}
