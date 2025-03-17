#include "Server.hpp"

int main() {
	Server	server(".");
	Server::HttpRequest request;
	server.parseRequest(std::cin, request);
}
