#include "catch.hpp"
#include <string>
#include <map>
#include <sstream>
#include <cstdlib>

// Mock HTTP request class
class HttpRequest {
public:
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;

    HttpRequest(const std::string& method, const std::string& path, const std::string& body = "")
        : method(method), path(path), version("HTTP/1.1"), body(body) {
        if (!body.empty()) {
            headers["Content-Length"] = std::to_string(body.length());
            headers["Content-Type"] = "text/plain";
        }
    }

    std::string toString() const {
        std::stringstream ss;
        ss << method << " " << path << " " << version << "\r\n";
        for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
            ss << it->first << ": " << it->second << "\r\n";
        }
        ss << "\r\n";
        if (!body.empty()) {
            ss << body;
        }
        return ss.str();
    }
};

// Mock HTTP response class
class HttpResponse {
public:
    int statusCode;
    std::string statusText;
    std::map<std::string, std::string> headers;
    std::string body;

    HttpResponse() : statusCode(0) {}

    static HttpResponse parse(const std::string& responseStr) {
        HttpResponse response;
        std::istringstream iss(responseStr);
        std::string line;

        // Parse status line
        if (std::getline(iss, line)) {
            size_t pos1 = line.find(' ');
            size_t pos2 = line.find(' ', pos1 + 1);
            if (pos1 != std::string::npos && pos2 != std::string::npos) {
                response.statusCode = std::atoi(line.substr(pos1 + 1, pos2 - pos1 - 1).c_str());
                response.statusText = line.substr(pos2 + 1);
            }
        }

        // Parse headers
        while (std::getline(iss, line) && !line.empty() && line != "\r") {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                std::string name = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                // Trim leading/trailing whitespace
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of("\r") + 1);
                response.headers[name] = value;
            }
        }

        // Parse body
        std::stringstream bodyStream;
        while (std::getline(iss, line)) {
            bodyStream << line << "\n";
        }
        response.body = bodyStream.str();

        return response;
    }
};

// Mock HTTP client class
class HttpClient {
public:
    static HttpResponse sendRequest(const HttpRequest& request) {
        // In a real test, this would send the request to the server
        // For now, we'll just return a mock response
        HttpResponse response;

        // List of supported methods
        std::string supportedMethods[] = {"GET", "HEAD", "POST", "DELETE"};
        bool methodSupported = false;

        for (int i = 0; i < 4; i++) {
            if (request.method == supportedMethods[i]) {
                methodSupported = true;
                break;
            }
        }

        if (!methodSupported) {
            response.statusCode = 405;
            response.statusText = "Method Not Allowed";
            response.headers["Content-Type"] = "text/html";
            response.headers["Allow"] = "GET, HEAD, POST, DELETE";
            response.body = "<html><body><h1>405 Method Not Allowed</h1></body></html>\n";
        } else {
            // Handle supported methods (simplified for this test)
            response.statusCode = 200;
            response.statusText = "OK";
            response.headers["Content-Type"] = "text/plain";
            response.body = "Request processed successfully\n";
        }

        return response;
    }
};

TEST_CASE("Unknown Request Tests") {
    SECTION("Test Unknown Method") {
        HttpRequest request("UNKNOWN", "/");
        HttpResponse response = HttpClient::sendRequest(request);

        REQUIRE(response.statusCode == 405);
        REQUIRE(response.statusText == "Method Not Allowed");
        REQUIRE(response.headers["Allow"] == "GET, HEAD, POST, DELETE");
    }

    SECTION("Test Malformed Method") {
        HttpRequest request("GET/", "/");
        request.method = "GET/"; // Explicitly set a malformed method
        HttpResponse response = HttpClient::sendRequest(request);

        REQUIRE(response.statusCode == 405);
        REQUIRE(response.statusText == "Method Not Allowed");
    }

    SECTION("Test Empty Method") {
        HttpRequest request("GET", "/");
        request.method = ""; // Explicitly set an empty method
        HttpResponse response = HttpClient::sendRequest(request);

        REQUIRE(response.statusCode == 405);
        REQUIRE(response.statusText == "Method Not Allowed");
    }

    SECTION("Test Method with Special Characters") {
        HttpRequest request("GET", "/");
        request.method = "GET!@#"; // Explicitly set a method with special characters
        HttpResponse response = HttpClient::sendRequest(request);

        REQUIRE(response.statusCode == 405);
        REQUIRE(response.statusText == "Method Not Allowed");
    }

    SECTION("Test Method with Lowercase Letters") {
        HttpRequest request("get", "/");
        HttpResponse response = HttpClient::sendRequest(request);

        REQUIRE(response.statusCode == 405);
        REQUIRE(response.statusText == "Method Not Allowed");
    }

    SECTION("Test Very Long Method") {
        std::string longMethod(1000, 'X');
        HttpRequest request(longMethod, "/");
        HttpResponse response = HttpClient::sendRequest(request);

        REQUIRE(response.statusCode == 405);
        REQUIRE(response.statusText == "Method Not Allowed");
    }
}
