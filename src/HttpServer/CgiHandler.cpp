#include "HttpServer.hpp"

#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <signal.h>
#include <sys/select.h>

bool HttpServer::isCgiScript(const std::string &path) {
    // Check if the file has a CGI extension
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return false;
    }

    std::string extension = path.substr(dotPos);
    return _cgiExtensions.find(extension) != _cgiExtensions.end();
}

std::string HttpServer::getCgiInterpreter(const std::string &path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "";
    }

    std::string extension = path.substr(dotPos);
    std::map<std::string, std::string>::iterator it = _cgiExtensions.find(extension);
    if (it == _cgiExtensions.end()) {
        return "";
    }

    return it->second;
}

std::map<std::string, std::string> HttpServer::buildCgiEnvironment(const std::string &path, const std::string &method, const std::string &query, size_t contentLength) {
    std::map<std::string, std::string> env;

    // Set up CGI environment variables
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    env["SERVER_SOFTWARE"] = "Webserv/1.0";

    // Use the first server configuration for SERVER_NAME and SERVER_PORT
    if (!_serverConfigs.empty()) {
        env["SERVER_NAME"] = _serverConfigs[0].address;

        std::ostringstream portStr;
        portStr << _serverConfigs[0].port;
        env["SERVER_PORT"] = portStr.str();
    } else {
        env["SERVER_NAME"] = "localhost";
        env["SERVER_PORT"] = "8080";
    }

    env["REQUEST_METHOD"] = method;
    env["SCRIPT_NAME"] = path;
    env["PATH_INFO"] = "";

    // Set QUERY_STRING if present
    env["QUERY_STRING"] = query;

    // Set CONTENT_LENGTH if present
    if (contentLength > 0) {
        std::ostringstream contentLengthStr;
        contentLengthStr << contentLength;
        env["CONTENT_LENGTH"] = contentLengthStr.str();
        env["CONTENT_TYPE"] = "application/x-www-form-urlencoded";
    }

    // Add some common environment variables
    env["PATH"] = "/usr/local/bin:/usr/bin:/bin";

    return env;
}

void HttpServer::executeCgi(int clientSocket, const std::string &path, const std::string &method, const std::string &query, const std::string &body, bool closeConnection) {
    log.info() << "Executing CGI script: " << path << std::endl;

    // Get the interpreter for this script
    std::string interpreter = getCgiInterpreter(path);
    if (interpreter.empty()) {
        log.error() << "No interpreter found for script: " << path << std::endl;

        std::string connectionHeader = closeConnection ? "close" : "keep-alive";
        std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\n"
                                   "Content-Type: text/html\r\n"
                                   "Content-Length: 144\r\n"
                                   "Connection: " + connectionHeader + "\r\n"
                                   "\r\n"
                                   "<html><head><title>500 Internal Server Error</title></head>"
                                   "<body><h1>500 Internal Server Error</h1>"
                                   "<p>No interpreter found for the requested CGI script.</p>"
                                   "</body></html>";

        queueWrite(clientSocket, errorResponse);
        return;
    }

    // Create pipes for communication with the CGI script
    int inputPipe[2];  // Server writes to CGI's stdin
    int outputPipe[2]; // Server reads from CGI's stdout

    if (pipe(inputPipe) < 0 || pipe(outputPipe) < 0) {
        log.error() << "Failed to create pipes for CGI execution" << std::endl;

        std::string connectionHeader = closeConnection ? "close" : "keep-alive";
        std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\n"
                                   "Content-Type: text/html\r\n"
                                   "Content-Length: 144\r\n"
                                   "Connection: " + connectionHeader + "\r\n"
                                   "\r\n"
                                   "<html><head><title>500 Internal Server Error</title></head>"
                                   "<body><h1>500 Internal Server Error</h1>"
                                   "<p>Failed to create pipes for CGI execution.</p>"
                                   "</body></html>";

        queueWrite(clientSocket, errorResponse);
        return;
    }

    // Build the CGI environment
    std::map<std::string, std::string> env = buildCgiEnvironment(path, method, query, body.length());

    // Fork a child process to execute the CGI script
    pid_t pid = fork();

    if (pid < 0) {
        // Fork failed
        log.error() << "Failed to fork for CGI execution" << std::endl;

        close(inputPipe[0]);
        close(inputPipe[1]);
        close(outputPipe[0]);
        close(outputPipe[1]);

        std::string connectionHeader = closeConnection ? "close" : "keep-alive";
        std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\n"
                                   "Content-Type: text/html\r\n"
                                   "Content-Length: 144\r\n"
                                   "Connection: " + connectionHeader + "\r\n"
                                   "\r\n"
                                   "<html><head><title>500 Internal Server Error</title></head>"
                                   "<body><h1>500 Internal Server Error</h1>"
                                   "<p>Failed to fork for CGI execution.</p>"
                                   "</body></html>";

        queueWrite(clientSocket, errorResponse);
        return;
    } else if (pid == 0) {
        // Child process

        // Close unused pipe ends
        close(inputPipe[1]);  // Close write end of input pipe
        close(outputPipe[0]); // Close read end of output pipe

        // Redirect stdin to input pipe
        if (dup2(inputPipe[0], STDIN_FILENO) < 0) {
            log.error() << "Failed to redirect stdin" << std::endl;
            exit(1);
        }
        close(inputPipe[0]);

        // Redirect stdout to output pipe
        if (dup2(outputPipe[1], STDOUT_FILENO) < 0) {
            log.error() << "Failed to redirect stdout" << std::endl;
            exit(1);
        }
        close(outputPipe[1]);

        // Set up environment variables
        for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it) {
            setenv(it->first.c_str(), it->second.c_str(), 1);
        }

        // Construct the full path to the script
        std::string scriptPath = "html/default" + path;

        // Execute the CGI script
        execl(interpreter.c_str(), interpreter.c_str(), scriptPath.c_str(), NULL);

        // If execl returns, there was an error
        log.error() << "Failed to execute CGI script" << std::endl;
        exit(1);
    } else {
        // Parent process

        // Close unused pipe ends
        close(inputPipe[0]);  // Close read end of input pipe
        close(outputPipe[1]); // Close write end of output pipe

        // Write request body to the CGI script's stdin if needed
        if (!body.empty()) {
            ssize_t bytesWritten = write(inputPipe[1], body.c_str(), body.length());
            if (bytesWritten <= 0) {
                log.error() << "Failed to write to CGI input pipe" << std::endl;
            }
        }
        close(inputPipe[1]); // Close write end after writing

        // Read the CGI script's output with a timeout
        char buffer[4096];
        ssize_t bytesRead;
        std::string cgiOutput;

        // Set up a timeout for reading from the pipe
        fd_set readSet;
        struct timeval timeout;
        int selectResult;

        // Set a timeout of 10 seconds for reading from the pipe
        const int readTimeout = 10;
        time_t readStartTime = time(NULL);

        while (true) {
            // Set up the file descriptor set for select
            FD_ZERO(&readSet);
            FD_SET(outputPipe[0], &readSet);

            // Set the timeout for select
            timeout.tv_sec = 1; // Check every second
            timeout.tv_usec = 0;

            // Wait for data to be available or timeout
            selectResult = select(outputPipe[0] + 1, &readSet, NULL, NULL, &timeout);

            // Check for select error
            if (selectResult < 0) {
                log.error() << "select failed" << std::endl;
                break;
            }

            // Check for timeout
            if (selectResult == 0) {
                // Check if we've exceeded the total read timeout
                if (time(NULL) - readStartTime > readTimeout) {
                    log.error() << "Reading from CGI script timed out after " << readTimeout << " seconds" << std::endl;
                    break;
                }
                continue; // Try again
            }

            // Data is available, read it
            bytesRead = read(outputPipe[0], buffer, sizeof(buffer) - 1);

            // Check for read error or end of file
            if (bytesRead <= 0) {
                break;
            }

            // Append the data to the output
            buffer[bytesRead] = '\0';
            cgiOutput.append(buffer);

            // Reset the read timeout start time since we're making progress
            readStartTime = time(NULL);
        }

        close(outputPipe[0]); // Close read end after reading

        // Wait for the child process to finish with a timeout
        int status;
        time_t startTime = time(NULL);
        pid_t result;

        // Set a timeout of 30 seconds for CGI execution
        const time_t cgiTimeout = 30;

        // Try to wait for the child process with WNOHANG to avoid blocking
        while ((result = waitpid(pid, &status, WNOHANG)) == 0) {
            // Check if we've exceeded the timeout
            if (time(NULL) - startTime > cgiTimeout) {
                log.error() << "CGI script execution timed out after " << cgiTimeout << " seconds" << std::endl;

                // Kill the child process
                kill(pid, SIGTERM);

                // Wait a bit for the process to terminate
                usleep(100000); // 100ms

                // If it's still running, force kill it
                if (waitpid(pid, &status, WNOHANG) == 0) {
                    log.error() << "CGI script did not terminate after SIGTERM, sending SIGKILL" << std::endl;
                    kill(pid, SIGKILL);
                    waitpid(pid, &status, 0); // Wait for the process to be cleaned up
                }

                // Send a timeout response to the client
                std::string connectionHeader = closeConnection ? "close" : "keep-alive";
                std::string timeoutResponse = "HTTP/1.1 504 Gateway Timeout\r\n"
                                           "Content-Type: text/html\r\n"
                                           "Content-Length: 166\r\n"
                                           "Connection: " + connectionHeader + "\r\n"
                                           "\r\n"
                                           "<html><head><title>504 Gateway Timeout</title></head>"
                                           "<body><h1>504 Gateway Timeout</h1>"
                                           "<p>The CGI script did not complete within the allowed time.</p>"
                                           "</body></html>";

                queueWrite(clientSocket, timeoutResponse);
                return;
            }

            // Sleep a bit to avoid busy waiting
            usleep(10000); // 10ms
        }

        // Check if waitpid failed
        if (result < 0) {
            log.error() << "waitpid failed" << std::endl;

            std::string connectionHeader = closeConnection ? "close" : "keep-alive";
            std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\n"
                                       "Content-Type: text/html\r\n"
                                       "Content-Length: 144\r\n"
                                       "Connection: " + connectionHeader + "\r\n"
                                       "\r\n"
                                       "<html><head><title>500 Internal Server Error</title></head>"
                                       "<body><h1>500 Internal Server Error</h1>"
                                       "<p>Failed to wait for CGI script completion.</p>"
                                       "</body></html>";

            queueWrite(clientSocket, errorResponse);
            return;
        }

        // Check if the CGI script executed successfully
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // Parse CGI output to separate headers and body
            size_t headerEnd = cgiOutput.find("\r\n\r\n");
            if (headerEnd == std::string::npos) {
                headerEnd = cgiOutput.find("\n\n");
            }

            std::string headers;
            std::string body;

            if (headerEnd != std::string::npos) {
                headers = cgiOutput.substr(0, headerEnd);
                body = cgiOutput.substr(headerEnd + (cgiOutput[headerEnd + 1] == '\n' ? 2 : 4));
            } else {
                // No headers found, treat the entire output as body
                body = cgiOutput;
            }

            // Check if headers include Content-Type
            bool hasContentType = headers.find("Content-Type:") != std::string::npos;

            // Prepare the HTTP response
            std::ostringstream response;
            response << "HTTP/1.1 200 OK\r\n";

            if (!headers.empty()) {
                // Add CGI headers
                response << headers << "\r\n";
            }

            if (!hasContentType) {
                // Add default Content-Type if not provided by the CGI script
                response << "Content-Type: text/html\r\n";
            }

            response << "Content-Length: " << body.length() << "\r\n";
            response << "Connection: " << (closeConnection ? "close" : "keep-alive") << "\r\n";
            response << "\r\n";
            response << body;

            // Send the response
            std::string responseStr = response.str();
            queueWrite(clientSocket, responseStr);
        } else {
            // CGI script execution failed
            log.error() << "CGI script execution failed with status: " << WEXITSTATUS(status) << std::endl;

            std::string connectionHeader = closeConnection ? "close" : "keep-alive";
            std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\n"
                                       "Content-Type: text/html\r\n"
                                       "Content-Length: 144\r\n"
                                       "Connection: " + connectionHeader + "\r\n"
                                       "\r\n"
                                       "<html><head><title>500 Internal Server Error</title></head>"
                                       "<body><h1>500 Internal Server Error</h1>"
                                       "<p>CGI script execution failed.</p>"
                                       "</body></html>";

            queueWrite(clientSocket, errorResponse);
        }
    }
}
