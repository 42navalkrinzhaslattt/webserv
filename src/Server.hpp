#pragma once

# include "Utils.hpp"

# include <iostream>
# include <algorithm>
# include <string>
# include <map>
# include <vector>
# include <sys/socket.h>
# include <netinet/in.h>
# include <fcntl.h>
# include <unistd.h>
# include <poll.h>

using std::string;
using std::map;
using std::vector;

class Server {
public:
	~Server();
	explicit Server(const string &confPath);

	// Server configuration structures
	struct LocationConfig {
		string path;
		string root;
		vector<string> index;
		bool autoindex;
		string redirect;
		vector<string> allowedMethods;
		size_t clientMaxBodySize;
		string uploadPath;
		map<string, string> cgiHandlers; // Map file extensions to CGI handlers

		LocationConfig() :
			path("/"),
			root("/tmp/www"),
			autoindex(false),
			clientMaxBodySize(1048576) {} // Default 1MB
	};

	struct ServerConfig {
		// Each server has one hostname and multiple ports
		string hostname;
		vector<int> ports;
		string root;
		vector<LocationConfig> locations;

		ServerConfig() : hostname("localhost"), root("/tmp/www") {}
	};

	struct HttpRequest;
	enum RequestState { READING_HEADERS, READING_BODY, REQUEST_COMPLETE, REQUEST_ERROR };
	struct HttpRequest {
		string method;
		string path;
		string rawQuery;
		string httpVersion;
		typedef map<string, string> Headers;
		Headers headers;
		string body;
		RequestState state;
		size_t contentLength;
		bool chunkedTransfer;
		size_t bytesRead;
		string temporaryBuffer;
		bool pathParsed;
		size_t thisChunkSize;

		HttpRequest()
				: method(), path("/"), rawQuery(), httpVersion(), headers(), body(), state(READING_HEADERS), contentLength(0),
				  chunkedTransfer(false), bytesRead(0), temporaryBuffer(), pathParsed(false),
				  thisChunkSize() {}
	};

	// Request parsing methods
	void	parseRequest(std::istream &input, HttpRequest &request);
	void	parseRequestLine(string &line, HttpRequest &request);
	void	parsePathAndQuery(string &str, HttpRequest &request);
	void	parseHeader(std::istream &input, HttpRequest &request);
	void	parseChunkedBody(std::istream &input, HttpRequest &request);

	// Server methods
	bool initialize(int port);
	void run();
	void stop();

	// HTTP method handlers
	void handleRequest(int clientFd, const HttpRequest& request);
	void handleGetRequest(int clientFd, const HttpRequest& request, LocationConfig* location, const string& physicalPath);
	void handlePostRequest(int clientFd, const HttpRequest& request, LocationConfig* location, const string& physicalPath);
	void handleDeleteRequest(int clientFd, const HttpRequest& request, const string& physicalPath);
	std::string getContentType(const std::string& path);

	// CGI handling
	bool isCgiRequest(const string& path, LocationConfig* location, string& cgiHandler);
	void handleCgiRequest(int clientFd, const HttpRequest& request, LocationConfig* location, const string& physicalPath, const string& cgiHandler);
	map<string, string> buildCgiEnvironment(const HttpRequest& request, const string& scriptPath, const string& pathInfo, const string& queryString);
	void sendCgiResponse(int clientFd, const string& cgiOutput);

	private:
	static const int MAX_CLIENTS = 1024;
	static const int CONNECTION_TIMEOUT = 10; // Timeout in seconds (reduced for testing)
	static const size_t INITIAL_BUFFER_SIZE = 4096;
	static const size_t MAX_BUFFER_SIZE = 1048576; // 1MB max buffer size

	vector<int> serverSockets; // Multiple server sockets for different ports
	map<int, int> socketPorts; // Map socket FD to port number
	map<int, int> clientPorts; // Map client FD to the port it connected to
	bool running;
	vector<pollfd> fds;
	map<int, HttpRequest> clientRequests;
	map<int, time_t> clientLastActivity; // Track last activity time for each client
	vector<ServerConfig> serverConfigs; // Server configuration from config file

	// Socket and connection handling
	bool setNonBlocking(int fd);
	void handleNewConnection(int serverSocket);
	void handleClientData(int clientFd);
	void removeClient(int clientFd);
	void sendResponse(int clientFd, const string& response);
	void checkTimeouts(); // Check for client timeouts
	void updateClientActivity(int clientFd); // Update client's last activity time
	void sendErrorResponse(int clientFd, int statusCode, const string& statusText, const string& errorMessage);
	string generateErrorPage(int statusCode, const string& statusText, const string& errorMessage);
	size_t getOptimalBufferSize(const HttpRequest& request); // Determine optimal buffer size based on request

	// Configuration and routing
	void loadConfig(const string& confPath); // Load configuration from file
	ServerConfig* matchServerConfig(const HttpRequest& request, const string& host, int port); // Match request to server block
	LocationConfig* matchLocationConfig(ServerConfig* serverConfig, const string& path); // Match request to location block
	string getPhysicalPath(const LocationConfig* location, const string& requestPath); // Get physical path from request path
	bool isMethodAllowed(const LocationConfig* location, const string& method); // Check if method is allowed
	bool handleRedirection(int clientFd, const LocationConfig* location); // Handle redirection if configured
	bool handleAutoindex(int clientFd, const LocationConfig* location, const string& physicalPath); // Handle directory listing
};
