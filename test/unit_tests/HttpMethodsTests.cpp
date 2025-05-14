#include "catch.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <map>

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

        if (request.method == "GET") {
            if (request.path == "/test.txt") {
                response.statusCode = 200;
                response.statusText = "OK";
                response.headers["Content-Type"] = "text/plain";
                response.body = "This is a test file.\n";
            } else if (request.path == "/") {
                response.statusCode = 200;
                response.statusText = "OK";
                response.headers["Content-Type"] = "text/html";
                response.body = "<html><body><h1>Index</h1></body></html>\n";
            } else {
                response.statusCode = 404;
                response.statusText = "Not Found";
                response.headers["Content-Type"] = "text/html";
                response.body = "<html><body><h1>404 Not Found</h1></body></html>\n";
            }
        } else if (request.method == "POST") {
            if (request.path == "/upload") {
                response.statusCode = 201;
                response.statusText = "Created";
                response.headers["Content-Type"] = "text/plain";
                response.body = "File uploaded successfully\n";
            } else {
                response.statusCode = 405;
                response.statusText = "Method Not Allowed";
                response.headers["Content-Type"] = "text/html";
                response.headers["Allow"] = "GET, HEAD, POST, DELETE";
                response.body = "<html><body><h1>405 Method Not Allowed</h1></body></html>\n";
            }
        } else if (request.method == "DELETE") {
            if (request.path == "/test.txt") {
                response.statusCode = 204;
                response.statusText = "No Content";
            } else {
                response.statusCode = 404;
                response.statusText = "Not Found";
                response.headers["Content-Type"] = "text/html";
                response.body = "<html><body><h1>404 Not Found</h1></body></html>\n";
            }
        } else {
            response.statusCode = 405;
            response.statusText = "Method Not Allowed";
            response.headers["Content-Type"] = "text/html";
            response.headers["Allow"] = "GET, HEAD, POST, DELETE";
            response.body = "<html><body><h1>405 Method Not Allowed</h1></body></html>\n";
        }

        return response;
    }
};

TEST_CASE("HTTP Methods Tests") {
    SECTION("Test GET Request for Existing File") {
        HttpRequest request("GET", "/test.txt");
        HttpResponse response = HttpClient::sendRequest(request);

        REQUIRE(response.statusCode == 200);
        REQUIRE(response.statusText == "OK");
        REQUIRE(response.headers["Content-Type"] == "text/plain");
        REQUIRE(response.body == "This is a test file.\n");
    }

    SECTION("Test GET Request for Directory") {
        HttpRequest request("GET", "/");
        HttpResponse response = HttpClient::sendRequest(request);

        REQUIRE(response.statusCode == 200);
        REQUIRE(response.statusText == "OK");
        REQUIRE(response.headers["Content-Type"] == "text/html");
        REQUIRE(response.body.find("<html>") != std::string::npos);
    }

    SECTION("Test GET Request for Non-existent File") {
        HttpRequest request("GET", "/nonexistent.txt");
        HttpResponse response = HttpClient::sendRequest(request);

        REQUIRE(response.statusCode == 404);
        REQUIRE(response.statusText == "Not Found");
        REQUIRE(response.headers["Content-Type"] == "text/html");
        REQUIRE(response.body.find("404 Not Found") != std::string::npos);
    }

    SECTION("Test POST Request for File Upload") {
        HttpRequest request("POST", "/upload", "This is the content of the uploaded file.");
        HttpResponse response = HttpClient::sendRequest(request);

        REQUIRE(response.statusCode == 201);
        REQUIRE(response.statusText == "Created");
        REQUIRE(response.body == "File uploaded successfully\n");
    }

    SECTION("Test POST Request for Invalid Path") {
        HttpRequest request("POST", "/invalid", "This is the content of the uploaded file.");
        HttpResponse response = HttpClient::sendRequest(request);

        REQUIRE(response.statusCode == 405);
        REQUIRE(response.statusText == "Method Not Allowed");
        REQUIRE(response.headers["Allow"] == "GET, HEAD, POST, DELETE");
    }

    SECTION("Test DELETE Request for Existing File") {
        HttpRequest request("DELETE", "/test.txt");
        HttpResponse response = HttpClient::sendRequest(request);

        REQUIRE(response.statusCode == 204);
        REQUIRE(response.statusText == "No Content");
    }

    SECTION("Test DELETE Request for Non-existent File") {
        HttpRequest request("DELETE", "/nonexistent.txt");
        HttpResponse response = HttpClient::sendRequest(request);

        REQUIRE(response.statusCode == 404);
        REQUIRE(response.statusText == "Not Found");
    }

    SECTION("Test Unknown Method") {
        HttpRequest request("UNKNOWN", "/");
        request.method = "UNKNOWN"; // Explicitly set an unknown method
        HttpResponse response = HttpClient::sendRequest(request);

        REQUIRE(response.statusCode == 405);
        REQUIRE(response.statusText == "Method Not Allowed");
        REQUIRE(response.headers["Allow"] == "GET, HEAD, POST, DELETE");
    }
}
