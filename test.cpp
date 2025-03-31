#include <iostream>
#include <cstring>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>

#define PORT 4243
#define BUFFER_SIZE 1024
#define BACKLOG 10

int	main() {
	struct addrinfo hints, *res;
	int	sockfd;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, "4243", &hints, &res)) {
		perror("getaddrinfo");
		return 1;
	}

	if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
		perror("socket");
		return 1;
	}

	if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
		perror("bind");
		return 1;
	}

	if (listen(sockfd, BACKLOG) == -1) {
		close(sockfd);
		perror("listen");
		return 1;
	}


	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof their_addr;
	char	buffer[BUFFER_SIZE];

	while (true) {
		int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		std::cout << "client connected!" << std::endl;

		while (true) {
			memset(buffer, 0, BUFFER_SIZE);
			ssize_t bytes_received = recv(new_fd, buffer, BUFFER_SIZE, 0);

			if (bytes_received == 0) {
				std::cout << "client disconnected" << std::endl;
				return 0;
			}

			if (bytes_received < 0) {
				perror("recv");
				return 1;
			}

			std::cout << "received " << bytes_received << " bytes" << std::endl;

			send(new_fd, buffer, bytes_received, 0);
		}

		close(new_fd);
	}
}
