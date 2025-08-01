# Webserv - HTTP Server Implementation

*"This is when you finally understand why URLs start with HTTP"*

## Overview

Webserv is a custom HTTP/1.1 server implementation written in C++98. This project provides hands-on experience with the HTTP protocol, socket programming, and server architecture. The server is designed to be non-blocking, efficient, and compatible with standard web browsers.

## Features

### Core HTTP Server
- **HTTP/1.1 Protocol Support**: Implements essential HTTP functionality
- **Non-blocking I/O**: Uses `poll()`, `select()`, `kqueue()`, or `epoll()` for efficient I/O multiplexing
- **Multi-port Support**: Can listen on multiple ports simultaneously
- **Static File Serving**: Serves static websites and files
- **File Upload Support**: Handles client file uploads
- **Method Support**: Implements GET, POST, and DELETE methods

### Configuration System
- **Flexible Configuration**: NGINX-inspired configuration file format
- **Virtual Hosts**: Support for multiple websites on different ports
- **Route-based Rules**: Define behavior per URL path
- **Error Pages**: Custom error page configuration
- **Request Size Limits**: Configurable maximum request body size

### Advanced Features
- **CGI Support**: Execute CGI scripts (PHP, Python, etc.)
- **Directory Listing**: Optional directory browsing
- **HTTP Redirections**: URL redirection support
- **Request Routing**: Path-based request handling
- **Error Handling**: Comprehensive error response system

## Project Structure

```
webserv/
├── src/                    # Source code
│   ├── main.cpp           # Entry point
│   ├── Server.cpp/.hpp    # Server implementation
│   ├── Config.cpp/.hpp    # Configuration parser
│   ├── Errors.cpp/.hpp    # Error handling
│   ├── ParseHeader.cpp    # HTTP header parsing
│   └── Utils.cpp/.hpp     # Utility functions
├── config/                # Configuration files
│   ├── default.conf       # Default server configuration
│   ├── cgi.conf          # CGI configuration
│   ├── routing.conf      # Route-specific settings
│   └── security.conf     # Security configurations
├── tests/                 # Test suite
│   └── unit/             # Unit tests
│       ├── config_tests/ # Configuration parser tests
│       └── http_tests/   # HTTP parsing tests
├── Makefile              # Build configuration
└── README.md            # This file
```

## Building and Installation

### Prerequisites
- C++ compiler with C++98 support
- Make utility
- Unix-like operating system (Linux/macOS)

### Compilation
```bash
# Clone the repository
git clone <repository-url>
cd webserv

# Build the project
make

# Clean build files
make clean

# Complete clean (removes executable)
make fclean

# Rebuild everything
make re
```

## Usage

### Basic Usage
```bash
# Run with default configuration
./webserv

# Run with custom configuration file
./webserv config/default.conf
```

### Configuration File Format

The configuration file uses an NGINX-inspired syntax:

```nginx
server {
    listen 8080;
    server_name localhost;
    
    # Error pages
    error_page 404 /errors/404.html;
    error_page 500 /errors/500.html;
    
    # Maximum request body size
    client_max_body_size 1M;
    
    # Root directory
    root /var/www/html;
    
    # Default index files
    index index.html index.htm;
    
    # Route configuration
    location / {
        allow_methods GET POST;
        autoindex on;
    }
    
    location /upload {
        allow_methods POST;
        upload_path /tmp/uploads;
    }
    
    location /cgi-bin {
        allow_methods GET POST;
        cgi_pass /usr/bin/php-cgi;
        cgi_extension .php;
    }
}
```

### Testing

#### Unit Tests
```bash
cd tests/unit
make
./config_test
./http_test
```

#### Integration Tests
```bash
# Test concurrent connections
./test_concurrent.sh

# Test path sanitization
python3 test_path_sanitization.py

# Manual testing with curl
curl -X GET http://localhost:8080/
curl -X POST -d "data=test" http://localhost:8080/form
curl -X DELETE http://localhost:8080/file.txt
```

#### Browser Testing
Open your web browser and navigate to:
- `http://localhost:8080` - Main website
- `http://localhost:8080/upload` - File upload form
- `http://localhost:8080/cgi-bin/test.php` - CGI script execution

## Configuration Options

### Server Block
- `listen`: Port number to listen on
- `server_name`: Server name (virtual host support)
- `root`: Document root directory
- `index`: Default index files
- `error_page`: Custom error pages
- `client_max_body_size`: Maximum request body size

### Location Block
- `allow_methods`: Allowed HTTP methods
- `autoindex`: Enable/disable directory listing
- `upload_path`: Directory for uploaded files
- `return`: HTTP redirection
- `cgi_pass`: CGI interpreter path
- `cgi_extension`: File extensions for CGI

## HTTP Methods

### GET
- Retrieve files and resources
- Support for query parameters
- Directory listing (when enabled)

### POST
- Form data submission
- File uploads
- CGI script execution

### DELETE
- File deletion (when permitted)
- Resource cleanup

## CGI Support

The server supports Common Gateway Interface (CGI) for dynamic content:

- **Supported Languages**: PHP, Python, Perl, etc.
- **Environment Variables**: Proper CGI environment setup
- **Request Handling**: Complete request data passed to CGI
- **Response Processing**: Handles CGI output including headers

### CGI Environment Variables
- `REQUEST_METHOD`
- `QUERY_STRING`
- `CONTENT_TYPE`
- `CONTENT_LENGTH`
- `PATH_INFO`
- `SCRIPT_NAME`
- And more...

## Error Handling

The server provides comprehensive error handling:

- **4xx Client Errors**: 400, 404, 405, 413, etc.
- **5xx Server Errors**: 500, 501, 502, etc.
- **Custom Error Pages**: Configurable error page templates
- **Graceful Degradation**: Continues operation despite errors

## Performance Features

- **Non-blocking I/O**: Efficient handling of multiple connections
- **Single Poll Loop**: All I/O operations through one poll/select call
- **Connection Management**: Proper handling of keep-alive and disconnections
- **Resource Cleanup**: Automatic cleanup of resources

## Security Considerations

- **Path Sanitization**: Prevents directory traversal attacks
- **Input Validation**: Validates HTTP requests and headers
- **Resource Limits**: Configurable limits on request size and connections
- **Safe CGI Execution**: Secure CGI script execution environment

## Testing and Validation

### Stress Testing
```bash
# Test server under load
./test_concurrent.sh

# Monitor server performance
top -p $(pgrep webserv)
```

### Compatibility Testing
- **Browsers**: Chrome, Firefox, Safari, Edge
- **HTTP Clients**: curl, wget, Postman
- **Comparison**: Tested against NGINX behavior

## Troubleshooting

### Common Issues

1. **Port Already in Use**
   ```bash
   # Check what's using the port
   lsof -i :8080
   # Kill existing process
   ./kill_server.sh
   ```

2. **Permission Denied**
   ```bash
   # Check file permissions
   ls -la /path/to/document/root
   # Ensure server has read access
   ```

3. **CGI Not Working**
   ```bash
   # Verify CGI interpreter path
   which php-cgi
   # Check script permissions
   chmod +x script.cgi
   ```

## Development

### Code Style
- C++98 standard compliance
- RAII principles
- Exception safety
- Clear separation of concerns

### Adding Features
1. Implement feature in appropriate source file
2. Add configuration parsing if needed
3. Write unit tests
4. Update documentation

### Debugging
```bash
# Compile with debug symbols
make DEBUG=1

# Run with debugging tools
valgrind ./webserv config/default.conf
gdb ./webserv
```

## License

This project is part of the 42 curriculum. Please respect academic integrity guidelines.

## Contributing

This is an educational project. While not accepting external contributions, feedback and suggestions are welcome for learning purposes.

## References

- [RFC 7230 - HTTP/1.1 Message Syntax and Routing](https://tools.ietf.org/html/rfc7230)
- [RFC 7231 - HTTP/1.1 Semantics and Content](https://tools.ietf.org/html/rfc7231)
- [CGI Specification](https://tools.ietf.org/html/rfc3875)
- [NGINX Configuration Guide](https://nginx.org/en/docs/)

---

*Understanding HTTP from the ground up - one request at a time.*
