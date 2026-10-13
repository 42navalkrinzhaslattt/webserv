# Webserv

An HTTP/1.1 server written from scratch in C++98: one non-blocking `poll()` event loop, multiple ports, NGINX-style configuration, static files, uploads, DELETE and CGI.

**Stack:** C++98 · POSIX sockets · poll · Make    **Context:** 42 Berlin curriculum

## Features

- **Non-blocking I/O** with a single `poll()` loop over all listening and client sockets
- **Multiple ports and server blocks**: one listening socket per unique port; requests are matched to a server block by port and `Host` header, falling back to the first block on that port
- **Location routing** by longest path prefix, with per-location root, index files, allowed methods, body size limit, upload directory, redirect and CGI handlers
- **GET**: static files with MIME types, index files, optional directory listing (autoindex)
- **POST**: writes the request body to the configured upload directory, or hands it to a CGI script
- **DELETE**: removes files under the location root
- **Chunked transfer encoding** and `Content-Length` bodies, parsed incrementally across reads
- **CGI** via `fork`/`execve` with pipes for stdin and stdout and a standard CGI environment (`REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_TYPE`, `CONTENT_LENGTH`, `SCRIPT_NAME`, `PATH_INFO`, `HTTP_*` headers and more)
- **HTTP redirects** (`return 301 /new-path;`)
- **Generated HTML error pages** for 400, 403, 404, 405 (with an `Allow` header), 408, 413, 500 and 501
- **Limits and timeouts**: 1 MB receive buffer cap, 1024 connections (503 above that), 10 s idle timeout (408)
- **Path sanitization**: URL decoding, `.`/`..` normalization and a check that the final path stays inside the location root

## How it works

`Server::run()` polls every socket with a 1 s timeout so idle clients can be timed out between events. A readable listening socket means `accept()` a new non-blocking client; a readable client socket means `recv()` into that client's buffer.

Each client has an `HttpRequest` state machine (`READING_HEADERS` -> `READING_BODY` -> `REQUEST_COMPLETE`). Once the headers are complete, the request line and headers are parsed (`src/ParseHeader.cpp`); chunked bodies are decoded directly, and `Content-Length` bodies are accumulated until all bytes have arrived. A complete request is routed to a server block and location, checked against the method list, body limit and redirect, mapped to a sanitized path on disk, and dispatched to the GET, POST or DELETE handler. The connection is then reset for the next request.

Responses are written with a `send()` loop, and CGI output is read from the child's pipe until EOF, so a slow CGI script blocks the loop while it runs.

## Build & run

Requires a C++98 compiler and Make on Linux or macOS.

```bash
make                 # builds ./webserv
make DEBUG=1 re      # debug symbols, -O0
make ASAN=1 re       # AddressSanitizer
make leakcheck       # rebuild and run under Valgrind
```

```bash
./webserv                        # uses config/default.conf
./webserv config/routing.conf    # any config file
```

The sample configs serve from `/tmp/www`, so create that directory and put some files in it first.

## Configuration

NGINX-inspired syntax, one directive per line:

```nginx
server {
    listen 8080 8081;            # one or more ports
    server_name localhost;
    root /tmp/www;

    location / {
        index index.html index.htm;
        autoindex on;
        limit_except GET POST DELETE;
        client_max_body_size 10485760;   # bytes
    }

    location /uploads {
        upload_path /tmp/www/uploads;
        limit_except POST;
    }

    location /cgi-bin {
        cgi_handler .py /usr/bin/python3;
        limit_except GET POST;
    }

    location /redirect {
        return 301 /;
    }
}
```

| Context  | Directive              | Meaning                                              |
| -------- | ---------------------- | ---------------------------------------------------- |
| server   | `listen`               | One or more ports                                    |
| server   | `server_name`          | Host name to match (first value is used)             |
| server   | `root`                 | Default document root, inherited by locations        |
| location | `root`                 | Document root for this location                      |
| location | `index`                | Index files to try for a directory                   |
| location | `autoindex`            | `on` / `off` directory listing                       |
| location | `limit_except`         | Allowed methods (all allowed if omitted)             |
| location | `client_max_body_size` | Maximum body size in bytes (default 1 MB)            |
| location | `upload_path`          | Directory where POST bodies are stored               |
| location | `cgi_handler`          | File extension and interpreter, e.g. `.py /usr/bin/python3` |
| location | `return`               | Redirect: `[code] url`                               |

Ready-made examples live in `config/`: `default.conf`, `routing.conf` (two servers, uploads, redirect), `cgi.conf` and `security.conf`.

## Testing

```bash
# Unit tests for the config parser and the HTTP header parser
cd tests/unit && make test

# With the server running on port 8080
./scripts/test_concurrent.sh              # 10 parallel curl requests
python3 scripts/test_path_sanitization.py # traversal and upload-path attacks (needs `requests`)
./scripts/kill_server.sh 8080             # free the port

curl http://localhost:8080/
curl -X POST --data-binary @file.txt http://localhost:8080/uploads/file.txt
curl -X DELETE http://localhost:8080/uploads/file.txt
```

## Project structure

```
webserv/
├── src/
│   ├── main.cpp          # Entry point, config path argument
│   ├── Server.cpp/.hpp   # Sockets, poll loop, routing, GET/POST/DELETE, CGI, error pages
│   ├── ParseHeader.cpp   # Request line, headers, chunked body
│   ├── Config.cpp/.hpp   # Tokenizing config parser with validation (covered by unit tests)
│   ├── Errors.cpp/.hpp   # Config error messages
│   └── Utils.cpp/.hpp    # String helpers, URL decoding, path normalization and safety checks
├── config/               # Sample configurations
├── tests/unit/           # Config and HTTP parser unit tests
├── scripts/              # Concurrency and path-traversal test scripts, port cleanup
└── Makefile
```

## References

- [RFC 7230: HTTP/1.1 Message Syntax and Routing](https://tools.ietf.org/html/rfc7230)
- [RFC 7231: HTTP/1.1 Semantics and Content](https://tools.ietf.org/html/rfc7231)
- [RFC 3875: CGI 1.1](https://tools.ietf.org/html/rfc3875)
