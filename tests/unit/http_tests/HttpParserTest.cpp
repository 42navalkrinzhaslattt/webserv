#include <iostream>
#include <string>
#include <cassert>
#include "../../src/HttpParser.hpp"

using namespace HTTP;

class HttpParserTest {
private:
    Parser parser;

    bool testRequestLine() {
        std::cout << "Testing request line parsing..." << std::endl;

        try {
            Request req = parser.parse(
                "GET /index.html HTTP/1.1\r\n"
                "\r\n"
            );

            if (req.request_line.method != GET) {
                std::cerr << "Wrong method parsed" << std::endl;
                return false;
            }
            if (req.request_line.target != "/index.html") {
                std::cerr << "Wrong target parsed" << std::endl;
                return false;
            }
            if (!(req.request_line.version == Version(1, 1))) {
                std::cerr << "Wrong version parsed" << std::endl;
                return false;
            }

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Unexpected exception: " << e.what() << std::endl;
            return false;
        }
    }

    bool testHeaders() {
        std::cout << "Testing header parsing..." << std::endl;

        try {
            Request req = parser.parse(
                "GET / HTTP/1.1\r\n"
                "Host: example.com\r\n"
                "Accept: text/html\r\n"
                "Accept: application/json\r\n"
                "content-type: text/plain\r\n"
                "\r\n"
            );

            // Test case-insensitive header names
            if (!req.hasHeader("HOST")) {
                std::cerr << "Case-insensitive header lookup failed" << std::endl;
                return false;
            }

            // Test duplicate headers
            vector<string> accepts = req.getHeader("Accept");
            if (accepts.size() != 2 || 
                accepts[0] != "text/html" || 
                accepts[1] != "application/json") {
                std::cerr << "Multiple header values not handled correctly" << std::endl;
                return false;
            }

            // Test header value
            if (req.getHeader("content-type")[0] != "text/plain") {
                std::cerr << "Wrong header value" << std::endl;
                return false;
            }

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Unexpected exception: " << e.what() << std::endl;
            return false;
        }
    }

    bool testChunkedBody() {
        std::cout << "Testing chunked body parsing..." << std::endl;

        try {
            Request req = parser.parse(
                "POST /upload HTTP/1.1\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n"
                "7\r\n"
                "Mozilla\r\n"
                "9\r\n"
                "Developer\r\n"
                "7\r\n"
                "Network\r\n"
                "0\r\n"
                "\r\n"
            );

            if (req.body != "MozillaDeveloperNetwork") {
                std::cerr << "Wrong chunked body parsed" << std::endl;
                return false;
            }

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Unexpected exception: " << e.what() << std::endl;
            return false;
        }
    }

    bool testContentLength() {
        std::cout << "Testing Content-Length body parsing..." << std::endl;

        try {
            Request req = parser.parse(
                "POST /submit HTTP/1.1\r\n"
                "Content-Length: 11\r\n"
                "\r\n"
                "Hello World"
            );

            if (req.body != "Hello World") {
                std::cerr << "Wrong Content-Length body parsed" << std::endl;
                return false;
            }

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Unexpected exception: " << e.what() << std::endl;
            return false;
        }
    }

    bool testMalformedRequests() {
        std::cout << "Testing malformed requests..." << std::endl;

        // Test invalid method
        try {
            parser.parse("INVALID / HTTP/1.1\r\n\r\n");
            std::cerr << "Should have thrown MethodNotAllowed" << std::endl;
            return false;
        } catch (const MethodNotAllowed&) {
            // Expected
        }

        // Test invalid HTTP version
        try {
            parser.parse("GET / HTTP/2.0\r\n\r\n");
            std::cerr << "Should have thrown UnsupportedVersion" << std::endl;
            return false;
        } catch (const UnsupportedVersion&) {
            // Expected
        }

        // Test malformed request line
        try {
            parser.parse("GET/HTTP/1.1\r\n\r\n");
            std::cerr << "Should have thrown MalformedRequest" << std::endl;
            return false;
        } catch (const MalformedRequest&) {
            // Expected
        }

        // Test invalid header format
        try {
            parser.parse(
                "GET / HTTP/1.1\r\n"
                "Invalid Header Line\r\n"
                "\r\n"
            );
            std::cerr << "Should have thrown MalformedRequest" << std::endl;
            return false;
        } catch (const MalformedRequest&) {
            // Expected
        }

        // Test multiple Content-Length headers
        try {
            parser.parse(
                "POST / HTTP/1.1\r\n"
                "Content-Length: 5\r\n"
                "Content-Length: 10\r\n"
                "\r\n"
                "Hello"
            );
            std::cerr << "Should have thrown MalformedRequest" << std::endl;
            return false;
        } catch (const MalformedRequest&) {
            // Expected
        }

        // Test invalid chunk size
        try {
            parser.parse(
                "POST / HTTP/1.1\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n"
                "XYZ\r\n"
                "Hello\r\n"
            );
            std::cerr << "Should have thrown MalformedRequest" << std::endl;
            return false;
        } catch (const MalformedRequest&) {
            // Expected
        }

        return true;
    }

public:
    bool runAllTests() {
        std::cout << "\nRunning HTTP Parser Tests...\n" << std::endl;

        std::cout << "1. Testing request line parsing..." << std::endl;
        if (!testRequestLine()) {
            std::cerr << "❌ Request line parsing test failed" << std::endl;
            return false;
        }
        std::cout << "✅ Request line parsing test passed" << std::endl;

        std::cout << "\n2. Testing header parsing..." << std::endl;
        if (!testHeaders()) {
            std::cerr << "❌ Header parsing test failed" << std::endl;
            return false;
        }
        std::cout << "✅ Header parsing test passed" << std::endl;

        std::cout << "\n3. Testing chunked body parsing..." << std::endl;
        if (!testChunkedBody()) {
            std::cerr << "❌ Chunked body parsing test failed" << std::endl;
            return false;
        }
        std::cout << "✅ Chunked body parsing test passed" << std::endl;

        std::cout << "\n4. Testing Content-Length body parsing..." << std::endl;
        if (!testContentLength()) {
            std::cerr << "❌ Content-Length body parsing test failed" << std::endl;
            return false;
        }
        std::cout << "✅ Content-Length body parsing test passed" << std::endl;

        std::cout << "\n5. Testing malformed requests..." << std::endl;
        if (!testMalformedRequests()) {
            std::cerr << "❌ Malformed requests test failed" << std::endl;
            return false;
        }
        std::cout << "✅ Malformed requests test passed" << std::endl;

        std::cout << "\nAll HTTP parser tests passed! ✅\n" << std::endl;
        return true;
    }
};

int main() {
    HttpParserTest test;
    return test.runAllTests() ? 0 : 1;
}
