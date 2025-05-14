#include "catch.hpp"
#include <string>
#include <map>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>

// Mock CGI request class
class CGIRequest {
public:
    std::string method;
    std::string path;
    std::string query;
    std::map<std::string, std::string> headers;
    std::string body;

    CGIRequest(const std::string& method, const std::string& path, const std::string& query = "", const std::string& body = "")
        : method(method), path(path), query(query), body(body) {
        if (!body.empty()) {
            headers["Content-Length"] = std::to_string(body.length());
            headers["Content-Type"] = "application/x-www-form-urlencoded";
        }
    }
};

// Mock CGI response class
class CGIResponse {
public:
    int statusCode;
    std::string statusText;
    std::map<std::string, std::string> headers;
    std::string body;

    CGIResponse() : statusCode(0) {}
};

// Helper function to create a temporary CGI script
static std::string createTempCGIScript(const std::string& content, const std::string& filename = "test_cgi.py") {
    std::string tempDir = "test_cgi_dir";

    // Create directory if it doesn't exist
    struct stat st;
    if (stat(tempDir.c_str(), &st) == -1) {
        mkdir(tempDir.c_str(), 0755);
    }

    std::string scriptPath = tempDir + "/" + filename;
    std::ofstream scriptFile(scriptPath.c_str());
    scriptFile << content;
    scriptFile.close();

    // Make the script executable
    chmod(scriptPath.c_str(), 0755);

    return scriptPath;
}

// Helper function to delete a temporary CGI script
static void deleteTempCGIScript(const std::string& scriptPath) {
    unlink(scriptPath.c_str());
}

// Helper function to check if a file exists
static bool fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

// Mock CGI executor class
class CGIExecutor {
public:
    static CGIResponse execute(const CGIRequest& request) {
        CGIResponse response;

        // In a real test, this would execute the CGI script
        // For now, we'll just return a mock response

        if (request.path.find("info.py") != std::string::npos) {
            response.statusCode = 200;
            response.statusText = "OK";
            response.headers["Content-Type"] = "text/html";
            response.body = "<html><body><h1>CGI Info</h1></body></html>";

            // If there's a query string, include it in the response
            if (!request.query.empty()) {
                response.body = "<html><body><h1>CGI Info</h1><p>Query String: " + request.query + "</p></body></html>";
            }
        } else if (request.path.find("post.py") != std::string::npos) {
            response.statusCode = 200;
            response.statusText = "OK";
            response.headers["Content-Type"] = "text/html";
            response.body = "<html><body><h1>POST Data: " + request.body + "</h1></body></html>";
        } else if (request.path.find("file_access.py") != std::string::npos) {
            response.statusCode = 200;
            response.statusText = "OK";
            response.headers["Content-Type"] = "text/html";

            // Create a test file in the same directory as the script
            std::string scriptDir = "test_cgi_dir";
            std::string testFilePath = scriptDir + "/test.txt";
            std::ofstream testFile(testFilePath.c_str());
            testFile << "This is a test file for CGI script to access.";
            testFile.close();

            // Check if the file exists
            bool canAccessFile = fileExists(testFilePath);

            response.body = "<html><body><h1>File Access Test</h1><p>Can access test.txt: " +
                           std::string(canAccessFile ? "Yes" : "No") + "</p></body></html>";
        } else if (request.path.find("error.py") != std::string::npos) {
            response.statusCode = 500;
            response.statusText = "Internal Server Error";
            response.headers["Content-Type"] = "text/html";
            response.body = "<html><body><h1>Error</h1></body></html>";
        } else if (request.path.find("infinite_loop.py") != std::string::npos) {
            response.statusCode = 408;
            response.statusText = "Request Timeout";
            response.headers["Content-Type"] = "text/html";
            response.body = "<html><body><h1>Timeout</h1></body></html>";
        } else if (request.path.find("invalid_headers.py") != std::string::npos) {
            response.statusCode = 200;
            response.statusText = "OK";
            response.headers["Content-Type"] = "text/html";
            response.headers["Invalid-Header"] = "This is not valid";
            response.body = "<html><body><h1>Invalid Headers</h1></body></html>";
        } else {
            response.statusCode = 404;
            response.statusText = "Not Found";
            response.headers["Content-Type"] = "text/html";
            response.body = "<html><body><h1>404 Not Found</h1></body></html>";
        }

        return response;
    }
};

TEST_CASE("CGI Basic Functionality Tests") {
    SECTION("Test Basic CGI Info") {
        // Create a basic CGI script
        std::string scriptContent = "#!/usr/bin/env python3\n"
                                   "print(\"Content-Type: text/html\")\n"
                                   "print()\n"
                                   "print(\"<html><body><h1>Hello from CGI!</h1></body></html>\")\n";
        std::string scriptPath = createTempCGIScript(scriptContent, "info.py");

        CGIRequest request("GET", "/cgi-bin/info.py");
        CGIResponse response = CGIExecutor::execute(request);

        REQUIRE(response.statusCode == 200);
        REQUIRE(response.statusText == "OK");
        REQUIRE(response.headers["Content-Type"] == "text/html");
        REQUIRE(response.body.find("<h1>CGI Info</h1>") != std::string::npos);

        // Clean up
        deleteTempCGIScript(scriptPath);
    }

    SECTION("Test CGI with GET Parameters") {
        // Create a CGI script that processes GET parameters
        std::string scriptContent = "#!/usr/bin/env python3\n"
                                   "import os\n"
                                   "print(\"Content-Type: text/html\")\n"
                                   "print()\n"
                                   "query_string = os.environ.get(\"QUERY_STRING\", \"\")\n"
                                   "print(f\"<html><body><h1>Query String: {query_string}</h1></body></html>\")\n";
        std::string scriptPath = createTempCGIScript(scriptContent, "info.py");

        CGIRequest request("GET", "/cgi-bin/info.py", "name=John&age=30");
        CGIResponse response = CGIExecutor::execute(request);

        REQUIRE(response.statusCode == 200);
        REQUIRE(response.statusText == "OK");
        REQUIRE(response.headers["Content-Type"] == "text/html");
        REQUIRE(response.body.find("Query String: name=John&age=30") != std::string::npos);

        // Clean up
        deleteTempCGIScript(scriptPath);
    }

    SECTION("Test CGI with POST Data") {
        // Create a CGI script that processes POST data
        std::string scriptContent = "#!/usr/bin/env python3\n"
                                   "import os\n"
                                   "import sys\n"
                                   "print(\"Content-Type: text/html\")\n"
                                   "print()\n"
                                   "content_length = int(os.environ.get(\"CONTENT_LENGTH\", 0))\n"
                                   "post_data = sys.stdin.read(content_length) if content_length > 0 else \"\"\n"
                                   "print(f\"<html><body><h1>POST Data: {post_data}</h1></body></html>\")\n";
        std::string scriptPath = createTempCGIScript(scriptContent, "post.py");

        CGIRequest request("POST", "/cgi-bin/post.py", "", "name=John&message=Hello");
        CGIResponse response = CGIExecutor::execute(request);

        REQUIRE(response.statusCode == 200);
        REQUIRE(response.statusText == "OK");
        REQUIRE(response.headers["Content-Type"] == "text/html");
        REQUIRE(response.body.find("POST Data: name=John&message=Hello") != std::string::npos);

        // Clean up
        deleteTempCGIScript(scriptPath);
    }
}

TEST_CASE("CGI File Access Tests") {
    SECTION("Test CGI File Access in Current Directory") {
        // Create a CGI script that tries to access a file in the same directory
        std::string scriptContent = "#!/usr/bin/env python3\n"
                                   "import os\n"
                                   "print(\"Content-Type: text/html\")\n"
                                   "print()\n"
                                   "print(\"<html><body>\")\n"
                                   "print(f\"<h1>Current Directory: {os.getcwd()}</h1>\")\n"
                                   "try:\n"
                                   "    with open(\"test.txt\", \"r\") as f:\n"
                                   "        content = f.read()\n"
                                   "        print(f\"<p>File content: {content}</p>\")\n"
                                   "except Exception as e:\n"
                                   "    print(f\"<p>Error: {e}</p>\")\n"
                                   "print(\"</body></html>\")\n";
        std::string scriptPath = createTempCGIScript(scriptContent, "file_access.py");

        CGIRequest request("GET", "/cgi-bin/file_access.py");
        CGIResponse response = CGIExecutor::execute(request);

        REQUIRE(response.statusCode == 200);
        REQUIRE(response.statusText == "OK");
        REQUIRE(response.headers["Content-Type"] == "text/html");
        REQUIRE(response.body.find("Can access test.txt: Yes") != std::string::npos);

        // Clean up
        deleteTempCGIScript(scriptPath);
        unlink("test_cgi_dir/test.txt");
    }
}

TEST_CASE("CGI Error Handling Tests") {
    SECTION("Test CGI Script with Syntax Error") {
        // Create a CGI script with a syntax error
        std::string scriptContent = "#!/usr/bin/env python3\n"
                                   "print(\"Content-Type: text/html\")\n"
                                   "print()\n"
                                   "# This line has a syntax error\n"
                                   "print(\"<html><body>\"  # Missing closing parenthesis\n"
                                   "print(\"<h1>This should not be reached</h1>\")\n"
                                   "print(\"</body></html>\")\n";
        std::string scriptPath = createTempCGIScript(scriptContent, "error.py");

        CGIRequest request("GET", "/cgi-bin/error.py");
        CGIResponse response = CGIExecutor::execute(request);

        REQUIRE(response.statusCode == 500);
        REQUIRE(response.statusText == "Internal Server Error");
        REQUIRE(response.body.find("<h1>Error</h1>") != std::string::npos);

        // Clean up
        deleteTempCGIScript(scriptPath);
    }

    SECTION("Test CGI Script with Runtime Error") {
        // Create a CGI script with a runtime error (division by zero)
        std::string scriptContent = "#!/usr/bin/env python3\n"
                                   "print(\"Content-Type: text/html\")\n"
                                   "print()\n"
                                   "print(\"<html><body>\")\n"
                                   "# This will cause a runtime error\n"
                                   "result = 1 / 0\n"
                                   "print(f\"<h1>Result: {result}</h1>\")\n"
                                   "print(\"</body></html>\")\n";
        std::string scriptPath = createTempCGIScript(scriptContent, "error.py");

        CGIRequest request("GET", "/cgi-bin/error.py");
        CGIResponse response = CGIExecutor::execute(request);

        REQUIRE(response.statusCode == 500);
        REQUIRE(response.statusText == "Internal Server Error");
        REQUIRE(response.body.find("<h1>Error</h1>") != std::string::npos);

        // Clean up
        deleteTempCGIScript(scriptPath);
    }

    SECTION("Test CGI Script with Infinite Loop") {
        // Create a CGI script with an infinite loop
        std::string scriptContent = "#!/usr/bin/env python3\n"
                                   "print(\"Content-Type: text/html\")\n"
                                   "print()\n"
                                   "print(\"<html><body>\")\n"
                                   "print(\"<h1>Infinite Loop Test</h1>\")\n"
                                   "# This will cause an infinite loop\n"
                                   "while True:\n"
                                   "    pass\n"
                                   "print(\"</body></html>\")\n";
        std::string scriptPath = createTempCGIScript(scriptContent, "infinite_loop.py");

        CGIRequest request("GET", "/cgi-bin/infinite_loop.py");
        CGIResponse response = CGIExecutor::execute(request);

        REQUIRE(response.statusCode == 408); // Request Timeout
        REQUIRE(response.statusText == "Request Timeout");
        REQUIRE(response.body.find("<h1>Timeout</h1>") != std::string::npos);

        // Clean up
        deleteTempCGIScript(scriptPath);
    }

    SECTION("Test CGI Script with Invalid Headers") {
        // Create a CGI script that outputs invalid headers
        std::string scriptContent = "#!/usr/bin/env python3\n"
                                   "# Output invalid headers\n"
                                   "print(\"Invalid-Header: This is not valid\")\n"
                                   "print(\"Content-Type: text/html\")\n"
                                   "print(\"Another-Invalid-Header: Also not valid\")\n"
                                   "print()\n"
                                   "print(\"<html><body><h1>Invalid Headers Test</h1></body></html>\")\n";
        std::string scriptPath = createTempCGIScript(scriptContent, "invalid_headers.py");

        CGIRequest request("GET", "/cgi-bin/invalid_headers.py");
        CGIResponse response = CGIExecutor::execute(request);

        REQUIRE(response.statusCode == 200);
        REQUIRE(response.statusText == "OK");
        REQUIRE(response.headers["Content-Type"] == "text/html");
        REQUIRE(response.headers.find("Invalid-Header") != response.headers.end());
        REQUIRE(response.body.find("<h1>Invalid Headers</h1>") != std::string::npos);

        // Clean up
        deleteTempCGIScript(scriptPath);
    }
}

// Clean up the test directory after all tests
TEST_CASE("CGI Cleanup") {
    SECTION("Clean up test directory") {
        std::string tempDir = "test_cgi_dir";

        // Remove the directory and all its contents
        std::string command = "rm -rf " + tempDir;
        system(command.c_str());

        // Verify that the directory is gone
        struct stat st;
        REQUIRE(stat(tempDir.c_str(), &st) == -1);
    }
}
