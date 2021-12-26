#include "log.h"

#include <netinet/in.h>
#include <thread>

#define PORT 9025

Logger::Logger()
{
	std::thread thread(&Logger::start_server, this);
}

Log Logger::log(std::string name)
{
	return Log(name, this);
}

int Logger::start_server()
{
	int socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketfd < 0) {
		return -1;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(PORT);

	if (bind(socketfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0)
	{
		return -2;
	}

	for (;;) {
		int connfd = accept(socketfd, NULL, NULL);

		this->mtx.lock();

		if (this->active_client > 0) {
			const char msg[] = "Another client connected, bye!";
			write(this->active_client, msg, sizeof(msg));
			close(this->active_client);
		}

		this->active_client = connfd;

		for (int i = 0; i < this->buffer.size(); i++) {
			write(this->active_client, this->buffer[i].c_str(), this->buffer[i].length());
		}
		this->buffer.clear();

		this->mtx.unlock();
	}

	return 0;
}

Log::Log(std::string name, Logger *logger)
{
	this->name = name;
	this->logger = logger;
	setvbuf(stdout, NULL, _IONBF, 0);
}
