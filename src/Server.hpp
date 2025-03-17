#pragma once

# include "Utils.hpp"

# include <iostream>
# include <algorithm>
# include <string>
# include <map>

using std::string;
using std::map;

class Server {
public:
	~Server();
	Server(const string &confPath);

	struct HttpRequest;
//	enum ChunkParsingState { PARSE_CHUNK, PARSE_CHUNK_SIZE };
//	enum RequestState { READING_HEADERS, READING_BODY, REQUEST_COMPLETE, REQUEST_ERROR };

	typedef map<string, string> Headers;

	struct HttpRequest {
		string method;
		string path;
		string rawQuery;
		string httpVersion;
		Headers headers;
		string body;
//		RequestState state;
		size_t contentLength;
		bool chunkedTransfer;
		size_t bytesRead;
		string temporaryBuffer;
		bool pathParsed;
//		ChunkParsingState chunkParsingState;
		size_t thisChunkSize;

		HttpRequest()
				: method(), path("/"), rawQuery(), httpVersion(), headers(), body(), contentLength(0),
				  chunkedTransfer(false), bytesRead(0), temporaryBuffer(), pathParsed(false),
				  thisChunkSize() {}
	};

	void	parseRequest(std::istream &input, HttpRequest &request);
	void	parseRequestLine(string &line, HttpRequest &request);
	void	parsePathAndQuery(string &str, HttpRequest &request);
	void	parseHeader(std::istream &input, HttpRequest &request);
	void	parseChunkedBody(std::istream &input, HttpRequest &request);

		private:
};
