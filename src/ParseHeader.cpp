#include "Server.hpp"
#include "Utils.hpp"

Server::Server(const std::string &confPath) {
	std::cout << "Conf path is " << confPath << std::endl;
}

Server::~Server() {}
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
		std::cerr << "error while parsing request line " << line << "\"" << std::endl;
		/* todo:set error flag */
		return ;
	}
	request.method = requestLineComponents[0];
	request.httpVersion = requestLineComponents[2];
	parsePathAndQuery(requestLineComponents[1], request);

	std::cout << "method: " << request.method << std::endl
			<< "HTTP version: " << request.httpVersion << std::endl
			<< "path: " << request.path << std::endl
			<< "query: " << request.rawQuery << std::endl;
}

void	Server::parseHeader(std::istream &input, Server::HttpRequest &request) {
	string line;

	while (true) {
		std::getline(input, line);
		if (line.empty())
			break;
		if (line[0] == '#') //comments in header
			continue;
		size_t pos;
		if ((pos = line.find(':')) == string::npos) {
			std::cerr << "error while parsing header line " << line << "$" << std::endl;
			/* todo:set error flag */
			return ;
		}
		string fieldName = line.substr(0, pos);
		string fieldValue = line.substr(pos + 1);
		Utils::ft_trim(fieldValue);
		if (request.headers[fieldName].empty())
			request.headers[fieldName] = fieldValue;
		else
			request.headers[fieldName] += ", " + fieldValue;
	}

	for (map<string, string>::iterator it = request.headers.begin(); it != request.headers.end(); ++it) {
		std::cout << it->first << ": " << it->second << '\n';
	}
}

void	Server::parseChunkedBody(std::istream &input, Server::HttpRequest &request) {
	string	line;
	size_t	length = 0;
	size_t	chunkSize = 0;
	char *endptr = NULL;

	std::getline(input, line);
	chunkSize = std::strtoul(line.c_str(), &endptr, 10);
	if (*endptr != 0) {
		std::cerr << "Error: invalid chunk size: " << line << std::endl;
		//todo: set an error flag
		return;
	}
	while (chunkSize > 0) {
		std::getline(input, line);
		request.body += line;
		length += chunkSize;
		std::getline(input, line);
		chunkSize = std::strtoul(line.c_str(), &endptr, 10);
		if (*endptr != 0) {
			std::cerr << "Error: invalid chunk size: " << line << std::endl;
			//todo: set an error flag
			return;
		}
	}
	//todo: trailer field
	request.contentLength = length;
	request.headers.erase("Transfer-Encoding");
}

void	Server::parseRequest(std::istream &input, Server::HttpRequest &request)
{
	string line;
	char *endptr = NULL;

	std::getline(input, line);
	Utils::ft_trim(line);
	Server::parseRequestLine(line, request);

	parseHeader(input, request);

	if (!request.headers["Transfer-Encoding"].empty() && !request.headers["Content-Length"].empty()) {
		std::cerr << "Error: Both Transfer-Encoding and Content-Length headers are present" << std::endl;
		/* todo:set error flag */
		return;
	}
	else if (!request.headers["Transfer-Encoding"].empty()) {
		string transferEncoding = request.headers["Transfer-Encoding"];

		if (transferEncoding != "chunked") {
			std::cerr << "Error: Unknown Transfer-Encoding" << std::endl;
			/* todo:set error flag */
			return;
		}
		parseChunkedBody(input, request);
	} else if ((request.contentLength = std::strtoul(request.headers["Content-Length"].c_str(), &endptr, 10)) == 0 || *endptr != 0) {
		if (*endptr != 0)
			std::cerr << "Error: Content-Length value is invalid" << std::endl;
		else
			std::cerr << "Error: Content-Length is 0" << std::endl;
		/* todo:set error flag */
		return;
	} else {
		request.body.resize(request.contentLength);
		input.read(&request.body[0], (std::streamsize)request.contentLength);
	}

	std::cout << "Message Body: " << std::endl << request.body << std::endl
		<< "Content-Length: " << request.contentLength << std::endl
		<< "Transfer-Encoding: " << request.headers["Transfer-Encoding"] << std::endl;
}
