#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include "Logger.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

// Forward declarations
class DirectoryIndexer;

// Type definitions
typedef std::vector<std::string> Arguments;
typedef std::vector<Arguments> ArgResults;
typedef std::pair<std::string, std::vector<Arguments> > LocationCtx;

// HTTP request structure
struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};

class HttpServer {
public:
    HttpServer(const std::string &configPath, Logger &log, bool onlyCheckConfig);
    ~HttpServer();

    void run();

private:
    bool setupServerSocket();
    void acceptNewConnections();
    void handleClientData(int clientSocket);
    void handleDeleteRequest(int clientSocket, const std::string &path, bool closeConnection = true);
    void handlePostRequest(int clientSocket, const std::string &request, const std::string &path, bool closeConnection = true);
    void parseMultipartFormData(const std::string &request, std::string &boundary, std::map<std::string, std::string> &formData);
    std::string extractFilename(const std::string &contentDisposition);
    void removeClientSocket(int clientSocket);

    // Request parsing methods
    HttpRequest parseHttpRequest(const std::string &requestStr);
    bool shouldCloseConnection(const HttpRequest &request);

    // Response sending methods
    void sendString(int clientSocket, const std::string &content, int statusCode = 200, const std::string &contentType = "", bool headOnly = false, bool closeConnection = true);
    bool sendFileContent(int clientSocket, const std::string &filePath, const LocationCtx &location, int statusCode = 200, const std::string &contentType = "", bool headOnly = false, bool closeConnection = true);

    // Static content serving
    bool serveStaticFile(int clientSocket, const std::string &path, const HttpRequest &request);
    void sendError(int clientSocket, int statusCode, const LocationCtx *location, bool closeConnection = true);

    // Redirect handling
    bool handleRedirect(int clientSocket, const HttpRequest &request, const LocationCtx &location);

    // Socket management methods
    void queueWrite(int clientSocket, const std::string &data);
    bool canWriteToSocket(int clientSocket);
    void closeAllSockets();
    bool setNonBlocking(int socket);
    bool setReuseAddr(int socket);

    // Upload handling methods
    std::string getFileName(const std::string &path);
    bool checkRequestBodySize(int clientSocket, const HttpRequest &request, size_t bodySize);
    void handleUpload(int clientSocket, const std::string &request, const std::string &path);
    void handleDelete(int clientSocket, const std::string &path);

    // CGI handling methods
    bool isCgiScript(const std::string &path);
    std::string getCgiInterpreter(const std::string &path);
    void executeCgi(int clientSocket, const std::string &path, const std::string &method, const std::string &query, const std::string &body, bool closeConnection = true);
    std::map<std::string, std::string> buildCgiEnvironment(const std::string &path, const std::string &method, const std::string &query, size_t contentLength);

    // Status text handling methods
    std::string getStatusText(int statusCode);

    // Configuration methods
    bool directiveExists(const std::vector<Arguments> &directives, const std::string &name) const;
    Arguments getFirstDirective(const std::vector<Arguments> &directives, const std::string &name) const;
    ArgResults getAllDirectives(const std::vector<Arguments> &directives, const std::string &name) const;
    const LocationCtx &requestToLocation(int clientSocket, const HttpRequest &request) const;

    // Configuration parsing methods
    bool validateConfig(const std::string &configPath);
    void parseConfig(const std::string &configPath);
    void parseServerBlock(const std::string &serverBlock);

    // Request handling methods
    void handleGetRequest(int clientSocket, const HttpRequest &request);
    std::string determineDiskPath(const HttpRequest &request, const LocationCtx &location);
    bool handleIndexes(int clientSocket, const std::string &diskPath, const HttpRequest &request, const LocationCtx &location);

    // Location matching methods
    void addLocation(const std::string &path, const std::vector<Arguments> &directives);
    void initDefaultLocation();

    // URI handling methods
    std::string canonicalizePath(const std::string &path);
    std::string decodeUri(const std::string &uri);
    std::string normalizeUri(const std::string &uri);

    // Timeout handling methods
    void initTimeouts();
    void updateCurrentTime();
    time_t getCurrentTime() const;
    void checkTimeouts();
    void updateClientActivity(int clientSocket);
    void setConnectionTimeout(time_t timeout);
    void setKeepAliveTimeout(time_t timeout);
    void setReadTimeout(time_t timeout);
    void setWriteTimeout(time_t timeout);

    // MIME type handling methods
    void initMimeTypes();
    std::string getMimeType(const std::string &path);

    // Status text handling methods
    void initStatusTexts();

    Logger &log;
    std::string _httpVersionString;
    std::set<int> _pendingCloses;

    // Server sockets
    struct ServerConfig {
        int socket;
        std::string address;
        int port;
        std::string serverName;
        std::vector<LocationCtx> locations;
    };
    std::vector<ServerConfig> _serverConfigs;

    // Client sockets
    std::set<int> _clientSockets;

    // Pending writes for client sockets
    std::map<int, std::string> _pendingWrites;

    // Server state
    bool _running;

    // CGI configuration
    std::map<std::string, std::string> _cgiExtensions; // Maps file extensions to interpreter paths

    // Location configuration
    std::vector<LocationCtx> _locations;
    std::vector<LocationCtx> _tempLocations;
    LocationCtx _defaultLocation;

    // MIME types
    std::map<std::string, std::string> _mimeTypes;

    // HTTP status texts
    std::map<int, std::string> _statusTexts;

    // Timeout configuration
    struct timeval _currentTime;
    time_t _connectionTimeout;
    time_t _keepAliveTimeout;
    time_t _readTimeout;
    time_t _writeTimeout;
    std::map<int, time_t> _clientLastActivity;
};

#endif // HTTP_SERVER_HPP

