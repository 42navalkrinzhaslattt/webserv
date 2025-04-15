#include "Server.hpp"
#include "Utils.hpp"

// GET /index.html?name=test HTTP/1.1

void Server::parsePathAndQuery(string &str, Server::HttpRequest &request) {
    if (str.find('?') == string::npos)
        request.path = str;
    else {
        request.path = str.substr(0, str.find('?'));
        request.rawQuery = str.substr(str.find('?') + 1);
    }
}

void Server::parseRequestLine(string &line, Server::HttpRequest &request) {
    vector<string> requestLineComponents = Utils::ft_split(line);

    if (requestLineComponents.size() != 3) {
        throw std::runtime_error("Invalid request line format");
    }
    request.method = requestLineComponents[0];
    request.httpVersion = requestLineComponents[2];
    parsePathAndQuery(requestLineComponents[1], request);

    std::cout << "method: " << request.method << std::endl
            << "HTTP version: " << request.httpVersion << std::endl
            << "path: " << request.path << std::endl
            << "query: " << request.rawQuery << std::endl;
}

void    Server::parseHeader(std::istream &input, Server::HttpRequest &request) {
    string line;

    while (true) {
        std::getline(input, line);
        // Handle CRLF line endings by removing any trailing \r
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line = line.substr(0, line.length() - 1);
        }

        if (line.empty())
            break;
        if (line[0] == '#') //comments in header
            continue;
        size_t pos;
        if ((pos = line.find(':')) == string::npos) {
            throw std::runtime_error("Invalid header format");
        }
        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);
        Utils::ft_trim(value);

        // Convert header keys to lowercase for case-insensitive comparison
        string lowerKey = key;
        for (size_t i = 0; i < lowerKey.length(); ++i) {
            lowerKey[i] = static_cast<char>(std::tolower(static_cast<int>(lowerKey[i])));
        }

        request.headers[lowerKey] = value;
    }

    for (map<string, string>::iterator it = request.headers.begin(); it != request.headers.end(); ++it) {
        std::cout << it->first << ": " << it->second << '\n';
    }
}

void Server::parseChunkedBody(std::istream &input, Server::HttpRequest &request) {
    string line;
    size_t length = 0;
    size_t chunkSize = 0;
    char *endptr = NULL;

    while (true) {
        std::getline(input, line);
        Utils::ft_trim(line);  // Remove any whitespace
        chunkSize = std::strtoul(line.c_str(), &endptr, 16);  // Parse as hex
        if (*endptr != 0) {
            throw std::runtime_error("Invalid chunk size");
        }

        if (chunkSize == 0)
            break;  // Last chunk

        // Read chunk data
        char* chunk = new char[chunkSize + 1];
        input.read(chunk, static_cast<long>(chunkSize));
        chunk[chunkSize] = '\0';
        request.body += chunk;
        delete[] chunk;

        length += chunkSize;

        // Read and discard CRLF
        std::getline(input, line);
    }

    //todo: trailer field
    request.contentLength = length;
    request.headers.erase("transfer-encoding");
}

void Server::parseRequest(std::istream &input, Server::HttpRequest &request)
{
    string line;
    char *endptr = NULL;

    // Only parse the request line and headers if we're in the READING_HEADERS state
    if (request.state == READING_HEADERS) {
        std::getline(input, line);
        Utils::ft_trim(line);
        Server::parseRequestLine(line, request);

        parseHeader(input, request);

        // Set content length and chunked transfer flags
        if (!request.headers["transfer-encoding"].empty()) {
            string transferEncoding = request.headers["transfer-encoding"];
            if (transferEncoding != "chunked") {
                throw std::runtime_error("Unknown Transfer-Encoding");
            }

            // Handle chunked transfer encoding directly
            std::string body;
            std::string chunkLine;
            size_t chunkSize = 0;

            // Debug output
            std::cout << "Processing chunked request..." << std::endl;

            // Read and process each chunk
            while (true) {
                // Read the chunk size line
                std::getline(input, chunkLine);
                Utils::ft_trim(chunkLine);

                // Debug output
                std::cout << "Chunk size line: '" << chunkLine << "'" << std::endl;

                // Parse the chunk size (hex)
                chunkSize = std::strtoul(chunkLine.c_str(), &endptr, 16);
                std::cout << "Parsed chunk size: " << chunkSize << std::endl;

                // If chunk size is 0, we're done
                if (chunkSize == 0) {
                    // Read and discard the final CRLF
                    std::getline(input, chunkLine);
                    break;
                }

                // Read the chunk data directly
                char* chunk = new char[chunkSize + 1];
                input.read(chunk, static_cast<std::streamsize>(chunkSize));
                chunk[chunkSize] = '\0';

                // Add the chunk data to the body
                body.append(chunk, chunkSize);

                // Debug output
                std::cout << "Read chunk: '" << chunk << "'" << std::endl;

                delete[] chunk;

                // Read and discard the CRLF after the chunk
                std::getline(input, chunkLine);
            }

            // Debug output
            std::cout << "Final body: '" << body << "'" << std::endl;

            request.body = body;
            request.contentLength = body.length();
            request.chunkedTransfer = false;
        } else if (!request.headers["content-length"].empty()) {
            request.contentLength = std::strtoul(request.headers["content-length"].c_str(), &endptr, 10);
            if (*endptr != 0) {
                throw std::runtime_error("Invalid Content-Length value");
            }
        } else {
            // No body (common for GET requests)
            request.contentLength = 0;
        }
    }

    // The actual reading of the body or chunks is now handled in handleClientData

    std::cout << "Message Body: " << std::endl << request.body << std::endl
        << "Content-Length: " << request.contentLength << std::endl
        << "Transfer-Encoding: " << request.headers["transfer-encoding"] << std::endl;
}
