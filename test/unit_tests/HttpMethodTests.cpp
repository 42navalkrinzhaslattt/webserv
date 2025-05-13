#include "catch.hpp"
#include "HttpServer.hpp"
#include "Logger.hpp"
#include <sstream>
#include <string>

// Helper class to expose private methods for testing
class TestHttpServer : public HttpServer {
public:
    TestHttpServer(const std::string &configPath, Logger &log, bool onlyCheckConfig)
        : HttpServer(configPath, log, onlyCheckConfig) {}
    
    // Expose the isMethodAllowed method
    bool testIsMethodAllowed(const std::string& method, const LocationCtx& location) {
        return isMethodAllowed(method, location);
    }
    
    // Expose the requestToLocation method
    const LocationCtx& testRequestToLocation(const HttpRequest& request) {
        return requestToLocation(0, request); // Use a dummy client socket
    }
};

TEST_CASE("HTTP Method Restriction Tests") {
    std::stringstream logStream;
    Logger log(logStream, "DEBUG");
    
    // Create a test config file
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
    
    std::string tempFilePath = "test_methods.conf";
    std::ofstream tempFile(tempFilePath.c_str());
    tempFile << configContent;
    tempFile.close();
    
    // Create a test server
    TestHttpServer server(tempFilePath, log, true);
    
    SECTION("Test Root Location Method Restrictions") {
        // Create a request for the root location
        HttpRequest request;
        request.path = "/";
        
        // Get the location for the request
        const LocationCtx& location = server.testRequestToLocation(request);
        
        // Test allowed methods
        REQUIRE(server.testIsMethodAllowed("GET", location) == true);
        REQUIRE(server.testIsMethodAllowed("POST", location) == true);
        REQUIRE(server.testIsMethodAllowed("DELETE", location) == true);
        
        // Test disallowed methods
        REQUIRE(server.testIsMethodAllowed("PUT", location) == false);
        REQUIRE(server.testIsMethodAllowed("PATCH", location) == false);
        REQUIRE(server.testIsMethodAllowed("HEAD", location) == false);
    }
    
    SECTION("Test /test Location Method Restrictions") {
        // Create a request for the /test location
        HttpRequest request;
        request.path = "/test";
        
        // Get the location for the request
        const LocationCtx& location = server.testRequestToLocation(request);
        
        // Test allowed methods
        REQUIRE(server.testIsMethodAllowed("GET", location) == true);
        
        // Test disallowed methods
        REQUIRE(server.testIsMethodAllowed("POST", location) == false);
        REQUIRE(server.testIsMethodAllowed("DELETE", location) == false);
        REQUIRE(server.testIsMethodAllowed("PUT", location) == false);
    }
    
    SECTION("Test /upload Location Method Restrictions") {
        // Create a request for the /upload location
        HttpRequest request;
        request.path = "/upload";
        
        // Get the location for the request
        const LocationCtx& location = server.testRequestToLocation(request);
        
        // Test allowed methods
        REQUIRE(server.testIsMethodAllowed("POST", location) == true);
        
        // Test disallowed methods
        REQUIRE(server.testIsMethodAllowed("GET", location) == false);
        REQUIRE(server.testIsMethodAllowed("DELETE", location) == false);
        REQUIRE(server.testIsMethodAllowed("PUT", location) == false);
    }
    
    // Clean up
    std::remove(tempFilePath.c_str());
}
