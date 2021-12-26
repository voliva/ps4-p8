#include "log.h"

#include <netinet/in.h>
#include <thread>

#define PORT 9025

// extern void debug_text(const char* str);

Logger::Logger()
{
}

Log Logger::log(std::string name)
{
	return Log(name, this);
}

void Logger::listen_clients()
{
	std::thread(&Logger::start_server, this);
}

int Logger::start_server()
{
	//debug_text("Starting");

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		return -1;
	}

	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(PORT);

	//debug_text("Binding");
	if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0)
	{
		return -2;
	}

	if (listen(sockfd, 5) != 0)
	{
		//debug_text("Failed to listen");
		//debug_text(strerror(errno));
		return -3;
	}

	struct sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);
	for (;;) {
		//debug_text("Listening");
		int connfd = accept(sockfd, (struct sockaddr*)&clientAddr, &addrLen);

		if (connfd < 0) {
			//debug_text("Failed to accept client");
			//debug_text(strerror(errno));
			return -1;
		}
		//debug_text("Accepted");

		int flag = 1;

		this->mtx.lock();

		if (this->active_client > 0) {
			/*debug_text("Client is active, checking error");
			int error = 0;
			socklen_t len = sizeof(error);
			int retval = getsockopt(this->active_client, SOL_SOCKET, SO_ERROR, &error, &len);
			if (retval != 0) {
				// there was a problem getting the error code
				debug_text("error getting socket error code");
				debug_text(strerror(retval));
				for (;;);
			}

			if (error != 0) {
				// socket has a non zero error status
				debug_text("socket error");
				debug_text(strerror(retval));
			}
			else {*/
				// debug_text("nothing happened, would send message");
				// TODO Crashes if the connection is broken, error checking above made no difference. MSG_NOSIGNAL made no difference.
			const char msg[] = "Another client connected, bye!";
			send(this->active_client, msg, sizeof(msg), MSG_NOSIGNAL);
			close(this->active_client);
			//}
		}

		this->active_client = connfd;

		const char msg[] = "Hello!";
		write(this->active_client, msg, sizeof(msg));

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
