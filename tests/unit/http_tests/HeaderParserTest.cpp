#include <iostream>
#include <sstream>
#include "Server.hpp"
#include "Utils.hpp"

class HeaderParserTest {
private:
    Server server;
    
    bool testRequestLineParsing() {
        std::cout << "Testing request line parsing..." << std::endl;
        
        try {
            Server::HttpRequest request;
            string line = "GET /index.html?param=value HTTP/1.1";
            server.parseRequestLine(line, request);
            
            if (request.method != "GET") {
                std::cerr << "Wrong method parsed. Expected: GET, Got: " << request.method << std::endl;
                return false;
            }
            if (request.path != "/index.html") {
                std::cerr << "Wrong path parsed. Expected: /index.html, Got: " << request.path << std::endl;
                return false;
            }
            if (request.rawQuery != "param=value") {
                std::cerr << "Wrong query parsed. Expected: param=value, Got: " << request.rawQuery << std::endl;
                return false;
            }
            if (request.httpVersion != "HTTP/1.1") {
                std::cerr << "Wrong HTTP version parsed. Expected: HTTP/1.1, Got: " << request.httpVersion << std::endl;
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Unexpected exception: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testHeaderParsing() {
        std::cout << "Testing header parsing..." << std::endl;
        
        try {
            std::stringstream input(
                "Host: example.com\n"
                "Content-Type: text/html\n"
                "Content-Length: 100\n"
                "Accept: */*\n"
                "\n"
            );
            
            Server::HttpRequest request;
            server.parseHeader(input, request);
            
            // Check headers
            if (request.headers["Host"] != "example.com") {
                std::cerr << "Wrong Host header. Expected: example.com, Got: " << request.headers["Host"] << std::endl;
                return false;
            }
            if (request.headers["Content-Type"] != "text/html") {
                std::cerr << "Wrong Content-Type header" << std::endl;
                return false;
            }
            if (request.headers["Content-Length"] != "100") {
                std::cerr << "Wrong Content-Length header" << std::endl;
                return false;
            }
            if (request.headers["Accept"] != "*/*") {
                std::cerr << "Wrong Accept header" << std::endl;
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Unexpected exception: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testChunkedBodyParsing() {
        std::cout << "Testing chunked body parsing..." << std::endl;
        
        try {
            std::stringstream input(
                "7\r\n"
                "Mozilla\r\n"
                "9\r\n"
                "Developer\r\n"
                "7\r\n"
                "Network\r\n"
                "0\r\n"
            );
            
            Server::HttpRequest request;
            server.parseChunkedBody(input, request);
            
            // Compare character by character to help debug
            string expected = "MozillaDeveloperNetwork";
            string actual = request.body;
            
            if (actual != expected) {
                std::cerr << "Wrong chunked body parsed.\n";
                std::cerr << "Expected " << expected.length() << " chars: ";
                for (size_t i = 0; i < expected.length(); ++i) {
                    std::cerr << (int)expected[i] << " ";
                }
                std::cerr << "\nGot " << actual.length() << " chars: ";
                for (size_t i = 0; i < actual.length(); ++i) {
                    std::cerr << (int)actual[i] << " ";
                }
                std::cerr << std::endl;
                return false;
            }
            
            if (request.contentLength != expected.length()) {
                std::cerr << "Wrong content length. Expected: " << expected.length() 
                         << ", Got: " << request.contentLength << std::endl;
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Unexpected exception: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testFullRequestParsing() {
        std::cout << "Testing full request parsing..." << std::endl;
        
        try {
            std::stringstream input(
                "POST /submit?id=123 HTTP/1.1\n"
                "Host: example.com\n"
                "Content-Type: text/plain\n"
                "Content-Length: 11\n"
                "\n"
                "Hello World"
            );
            
            Server::HttpRequest request;
            server.parseRequest(input, request);
            
            // Check request line
            if (request.method != "POST" || 
                request.path != "/submit" || 
                request.rawQuery != "id=123" || 
                request.httpVersion != "HTTP/1.1") {
                std::cerr << "Request line parsing failed" << std::endl;
                return false;
            }
            
            // Check headers
            if (request.headers["Host"] != "example.com" || 
                request.headers["Content-Type"] != "text/plain" || 
                request.headers["Content-Length"] != "11") {
                std::cerr << "Header parsing failed" << std::endl;
                return false;
            }
            
            // Check body
            if (request.body != "Hello World") {
                std::cerr << "Body parsing failed. Expected: Hello World, Got: " << request.body << std::endl;
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Unexpected exception: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testErrorCases() {
        std::cout << "Testing error cases..." << std::endl;
        
        // Test invalid request line
        try {
            Server::HttpRequest request;
            string line = "GET";  // Missing path and version
            server.parseRequestLine(line, request);
            std::cerr << "Should have failed on invalid request line" << std::endl;
            return false;
        } catch (...) {
            // Expected
        }
        
        // Test malformed header
        try {
            std::stringstream input(
                "Invalid Header Line\r\n"
                "\r\n"
            );
            Server::HttpRequest request;
            server.parseHeader(input, request);
            std::cerr << "Should have failed on malformed header" << std::endl;
            return false;
        } catch (...) {
            // Expected
        }
        
        // Test invalid chunk size
        try {
            std::stringstream input(
                "XYZ\r\n"  // Invalid hex number
                "data\r\n"
                "0\r\n"
                "\r\n"
            );
            Server::HttpRequest request;
            server.parseChunkedBody(input, request);
            std::cerr << "Should have failed on invalid chunk size" << std::endl;
            return false;
        } catch (...) {
            // Expected
        }
        
        // Test conflicting transfer headers
        try {
            std::stringstream input(
                "POST / HTTP/1.1\r\n"
                "Content-Length: 10\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n"
                "data"
            );
            Server::HttpRequest request;
            server.parseRequest(input, request);
            std::cerr << "Should have failed on conflicting transfer headers" << std::endl;
            return false;
        } catch (...) {
            // Expected
        }
        
        return true;
    }

public:
    HeaderParserTest() : server("") {}  // Initialize with empty config path
    
    bool runAllTests() {
        std::cout << "\nRunning HTTP Header Parser Tests...\n" << std::endl;
        
        std::cout << "1. Testing request line parsing..." << std::endl;
        if (!testRequestLineParsing()) {
            std::cerr << "❌ Request line parsing test failed" << std::endl;
            return false;
        }
        std::cout << "✅ Request line parsing test passed" << std::endl;
        
        std::cout << "\n2. Testing header parsing..." << std::endl;
        if (!testHeaderParsing()) {
            std::cerr << "❌ Header parsing test failed" << std::endl;
            return false;
        }
        std::cout << "✅ Header parsing test passed" << std::endl;
        
        std::cout << "\n3. Testing chunked body parsing..." << std::endl;
        if (!testChunkedBodyParsing()) {
            std::cerr << "❌ Chunked body parsing test failed" << std::endl;
            return false;
        }
        std::cout << "✅ Chunked body parsing test passed" << std::endl;
        
        std::cout << "\n4. Testing full request parsing..." << std::endl;
        if (!testFullRequestParsing()) {
            std::cerr << "❌ Full request parsing test failed" << std::endl;
            return false;
        }
        std::cout << "✅ Full request parsing test passed" << std::endl;
        
        std::cout << "\n5. Testing error cases..." << std::endl;
        if (!testErrorCases()) {
            std::cerr << "❌ Error cases test failed" << std::endl;
            return false;
        }
        std::cout << "✅ Error cases test passed" << std::endl;
        
        std::cout << "\nAll HTTP header parser tests passed! ✅\n" << std::endl;
        return true;
    }
};

int main() {
    HeaderParserTest test;
    return test.runAllTests() ? 0 : 1;
}
