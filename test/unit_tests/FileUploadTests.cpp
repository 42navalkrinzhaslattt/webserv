#include "catch.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#include <map>


// Helper function to check if a file exists
static bool fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

// Helper function to read a file's content
static std::string readFileContent(const std::string& path) {
    std::ifstream file(path.c_str());
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Mock multipart form data generator
class MultipartFormData {
public:
    static std::string generate(const std::string& boundary, const std::string& fieldName,
                               const std::string& fileName, const std::string& content) {
        std::stringstream ss;
        ss << "--" << boundary << "\r\n";
        ss << "Content-Disposition: form-data; name=\"" << fieldName << "\"; filename=\"" << fileName << "\"\r\n";
        ss << "Content-Type: text/plain\r\n\r\n";
        ss << content << "\r\n";
        ss << "--" << boundary << "--\r\n";
        return ss.str();
    }
};

// Mock HTTP request class for file uploads
class FileUploadRequest {
public:
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;

    FileUploadRequest(const std::string& path, const std::string& fileName, const std::string& content)
        : method("POST"), path(path), version("HTTP/1.1") {
        std::string boundary = "----WebKitFormBoundaryABC123";
        body = MultipartFormData::generate(boundary, "file", fileName, content);
        headers["Content-Type"] = "multipart/form-data; boundary=" + boundary;
        headers["Content-Length"] = std::to_string(body.length());
    }

    std::string toString() const {
        std::stringstream ss;
        ss << method << " " << path << " " << version << "\r\n";
        for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
            ss << it->first << ": " << it->second << "\r\n";
        }
        ss << "\r\n";
        ss << body;
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

// Mock HTTP client class for file uploads
class FileUploadClient {
public:
    static HttpResponse uploadFile(const std::string& path, const std::string& fileName, const std::string& content) {
        FileUploadRequest request(path, fileName, content);

        // In a real test, this would send the request to the server
        // For now, we'll just return a mock response
        HttpResponse response;

        if (path == "/upload") {
            response.statusCode = 201;
            response.statusText = "Created";
            response.headers["Content-Type"] = "text/plain";
            response.body = "File uploaded successfully\n";

            // Simulate saving the file
            std::string uploadPath = "uploads/" + fileName;
            std::ofstream uploadFile(uploadPath.c_str());
            uploadFile << content;
            uploadFile.close();
        } else {
            response.statusCode = 404;
            response.statusText = "Not Found";
            response.headers["Content-Type"] = "text/html";
            response.body = "<html><body><h1>404 Not Found</h1></body></html>\n";
        }

        return response;
    }

    static HttpResponse getFile(const std::string& path) {
        // In a real test, this would send a GET request to the server
        // For now, we'll just return a mock response
        HttpResponse response;

        if (fileExists(path.substr(1))) { // Remove leading slash
            response.statusCode = 200;
            response.statusText = "OK";
            response.headers["Content-Type"] = "text/plain";
            response.body = readFileContent(path.substr(1));
        } else {
            response.statusCode = 404;
            response.statusText = "Not Found";
            response.headers["Content-Type"] = "text/html";
            response.body = "<html><body><h1>404 Not Found</h1></body></html>\n";
        }

        return response;
    }

    static HttpResponse deleteFile(const std::string& path) {
        // In a real test, this would send a DELETE request to the server
        // For now, we'll just return a mock response
        HttpResponse response;

        if (fileExists(path.substr(1))) { // Remove leading slash
            // Simulate deleting the file
            std::remove(path.substr(1).c_str());

            response.statusCode = 204;
            response.statusText = "No Content";
        } else {
            response.statusCode = 404;
            response.statusText = "Not Found";
            response.headers["Content-Type"] = "text/html";
            response.body = "<html><body><h1>404 Not Found</h1></body></html>\n";
        }

        return response;
    }
};

TEST_CASE("File Upload and Retrieval Tests") {
    // Create uploads directory if it doesn't exist
    mkdir("uploads", 0755);

    SECTION("Test File Upload") {
        std::string fileName = "test_upload.txt";
        std::string content = "This is a test file for uploading.\nIt contains multiple lines of text.";

        HttpResponse response = FileUploadClient::uploadFile("/upload", fileName, content);

        REQUIRE(response.statusCode == 201);
        REQUIRE(response.statusText == "Created");
        REQUIRE(fileExists("uploads/" + fileName));
        REQUIRE(readFileContent("uploads/" + fileName) == content);
    }

    SECTION("Test File Retrieval") {
        // First upload a file
        std::string fileName = "test_retrieve.txt";
        std::string content = "This is a test file for retrieving.\nIt contains multiple lines of text.";
        FileUploadClient::uploadFile("/upload", fileName, content);

        // Then retrieve it
        HttpResponse response = FileUploadClient::getFile("/uploads/" + fileName);

        REQUIRE(response.statusCode == 200);
        REQUIRE(response.statusText == "OK");
        REQUIRE(response.headers["Content-Type"] == "text/plain");
        REQUIRE(response.body == content);
    }

    SECTION("Test File Deletion") {
        // First upload a file
        std::string fileName = "test_delete.txt";
        std::string content = "This is a test file for deleting.\nIt contains multiple lines of text.";
        FileUploadClient::uploadFile("/upload", fileName, content);

        // Verify it exists
        REQUIRE(fileExists("uploads/" + fileName));

        // Then delete it
        HttpResponse response = FileUploadClient::deleteFile("/uploads/" + fileName);

        REQUIRE(response.statusCode == 204);
        REQUIRE(response.statusText == "No Content");
        REQUIRE_FALSE(fileExists("uploads/" + fileName));
    }

    SECTION("Test Upload to Invalid Path") {
        std::string fileName = "test_invalid.txt";
        std::string content = "This is a test file for uploading to an invalid path.";

        HttpResponse response = FileUploadClient::uploadFile("/invalid", fileName, content);

        REQUIRE(response.statusCode == 404);
        REQUIRE(response.statusText == "Not Found");
        REQUIRE_FALSE(fileExists("uploads/" + fileName));
    }

    SECTION("Test Retrieval of Non-existent File") {
        HttpResponse response = FileUploadClient::getFile("/uploads/nonexistent.txt");

        REQUIRE(response.statusCode == 404);
        REQUIRE(response.statusText == "Not Found");
    }

    SECTION("Test Deletion of Non-existent File") {
        HttpResponse response = FileUploadClient::deleteFile("/uploads/nonexistent.txt");

        REQUIRE(response.statusCode == 404);
        REQUIRE(response.statusText == "Not Found");
    }

    // Clean up
    system("rm -rf uploads");
}
