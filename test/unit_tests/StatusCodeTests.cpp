#include "catch.hpp"
#include <string>
#include <map>

// Mock HTTP response class
class HttpResponse {
public:
    int statusCode;
    std::string statusText;
    std::map<std::string, std::string> headers;
    std::string body;

    HttpResponse(int code, const std::string& text) : statusCode(code), statusText(text) {}
};

// Mock HTTP status code generator
class StatusCodeGenerator {
public:
    static HttpResponse generate(int statusCode) {
        std::map<int, std::string> statusTexts;
        
        // 1xx - Informational
        statusTexts[100] = "Continue";
        statusTexts[101] = "Switching Protocols";
        
        // 2xx - Success
        statusTexts[200] = "OK";
        statusTexts[201] = "Created";
        statusTexts[204] = "No Content";
        
        // 3xx - Redirection
        statusTexts[301] = "Moved Permanently";
        statusTexts[302] = "Found";
        statusTexts[304] = "Not Modified";
        
        // 4xx - Client Error
        statusTexts[400] = "Bad Request";
        statusTexts[401] = "Unauthorized";
        statusTexts[403] = "Forbidden";
        statusTexts[404] = "Not Found";
        statusTexts[405] = "Method Not Allowed";
        statusTexts[408] = "Request Timeout";
        statusTexts[413] = "Payload Too Large";
        
        // 5xx - Server Error
        statusTexts[500] = "Internal Server Error";
        statusTexts[501] = "Not Implemented";
        statusTexts[502] = "Bad Gateway";
        statusTexts[503] = "Service Unavailable";
        statusTexts[504] = "Gateway Timeout";
        
        if (statusTexts.find(statusCode) != statusTexts.end()) {
            return HttpResponse(statusCode, statusTexts[statusCode]);
        } else {
            return HttpResponse(statusCode, "Unknown Status");
        }
    }
};

TEST_CASE("HTTP Status Code Tests") {
    SECTION("Test 1xx Informational Status Codes") {
        HttpResponse response100 = StatusCodeGenerator::generate(100);
        REQUIRE(response100.statusCode == 100);
        REQUIRE(response100.statusText == "Continue");
        
        HttpResponse response101 = StatusCodeGenerator::generate(101);
        REQUIRE(response101.statusCode == 101);
        REQUIRE(response101.statusText == "Switching Protocols");
    }
    
    SECTION("Test 2xx Success Status Codes") {
        HttpResponse response200 = StatusCodeGenerator::generate(200);
        REQUIRE(response200.statusCode == 200);
        REQUIRE(response200.statusText == "OK");
        
        HttpResponse response201 = StatusCodeGenerator::generate(201);
        REQUIRE(response201.statusCode == 201);
        REQUIRE(response201.statusText == "Created");
        
        HttpResponse response204 = StatusCodeGenerator::generate(204);
        REQUIRE(response204.statusCode == 204);
        REQUIRE(response204.statusText == "No Content");
    }
    
    SECTION("Test 3xx Redirection Status Codes") {
        HttpResponse response301 = StatusCodeGenerator::generate(301);
        REQUIRE(response301.statusCode == 301);
        REQUIRE(response301.statusText == "Moved Permanently");
        
        HttpResponse response302 = StatusCodeGenerator::generate(302);
        REQUIRE(response302.statusCode == 302);
        REQUIRE(response302.statusText == "Found");
        
        HttpResponse response304 = StatusCodeGenerator::generate(304);
        REQUIRE(response304.statusCode == 304);
        REQUIRE(response304.statusText == "Not Modified");
    }
    
    SECTION("Test 4xx Client Error Status Codes") {
        HttpResponse response400 = StatusCodeGenerator::generate(400);
        REQUIRE(response400.statusCode == 400);
        REQUIRE(response400.statusText == "Bad Request");
        
        HttpResponse response401 = StatusCodeGenerator::generate(401);
        REQUIRE(response401.statusCode == 401);
        REQUIRE(response401.statusText == "Unauthorized");
        
        HttpResponse response403 = StatusCodeGenerator::generate(403);
        REQUIRE(response403.statusCode == 403);
        REQUIRE(response403.statusText == "Forbidden");
        
        HttpResponse response404 = StatusCodeGenerator::generate(404);
        REQUIRE(response404.statusCode == 404);
        REQUIRE(response404.statusText == "Not Found");
        
        HttpResponse response405 = StatusCodeGenerator::generate(405);
        REQUIRE(response405.statusCode == 405);
        REQUIRE(response405.statusText == "Method Not Allowed");
        
        HttpResponse response408 = StatusCodeGenerator::generate(408);
        REQUIRE(response408.statusCode == 408);
        REQUIRE(response408.statusText == "Request Timeout");
        
        HttpResponse response413 = StatusCodeGenerator::generate(413);
        REQUIRE(response413.statusCode == 413);
        REQUIRE(response413.statusText == "Payload Too Large");
    }
    
    SECTION("Test 5xx Server Error Status Codes") {
        HttpResponse response500 = StatusCodeGenerator::generate(500);
        REQUIRE(response500.statusCode == 500);
        REQUIRE(response500.statusText == "Internal Server Error");
        
        HttpResponse response501 = StatusCodeGenerator::generate(501);
        REQUIRE(response501.statusCode == 501);
        REQUIRE(response501.statusText == "Not Implemented");
        
        HttpResponse response502 = StatusCodeGenerator::generate(502);
        REQUIRE(response502.statusCode == 502);
        REQUIRE(response502.statusText == "Bad Gateway");
        
        HttpResponse response503 = StatusCodeGenerator::generate(503);
        REQUIRE(response503.statusCode == 503);
        REQUIRE(response503.statusText == "Service Unavailable");
        
        HttpResponse response504 = StatusCodeGenerator::generate(504);
        REQUIRE(response504.statusCode == 504);
        REQUIRE(response504.statusText == "Gateway Timeout");
    }
    
    SECTION("Test Unknown Status Code") {
        HttpResponse response999 = StatusCodeGenerator::generate(999);
        REQUIRE(response999.statusCode == 999);
        REQUIRE(response999.statusText == "Unknown Status");
    }
}
