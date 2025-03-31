#include "Server.hpp"
#include "Config.hpp"

int main(int argc, char **argv) {
	Server	server(".");
	Server::HttpRequest request;
	server.parseRequest(std::cin, request);
	parseConfig(argv[1]);
	(void)argc;
}
