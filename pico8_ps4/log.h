#pragma once

#include <string>
#include <mutex>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>

#include "circular_buffer.h"

class Log;

class Logger
{
public:
	Logger();

	Log log(std::string name);
	void listen_clients();
	int start_server();

	Logger& operator<<(const char* v) {
		printf("%s", v);

		this->mtx.lock();

		this->buffer->push(v);
		if (this->active_client > 0) {
			send(this->active_client, v, sizeof(v), MSG_NOSIGNAL);
		}

		this->mtx.unlock();

		return *this;
	}
	Logger& operator<<(std::string &v) {
		return (*this << v.c_str());
	}
	template <class T>
	Logger& operator<<(const T& v) {
		std::string string_value = std::to_string(v);
		return (*this << string_value);
	}

private:
	std::mutex mtx;
	CircularBuffer<std::string> *buffer;
	int active_client;
};

class Log
{
private:
	std::string name;
	Logger* logger;

public:
	Log(std::string name, Logger *logger);

	template <class T>
	Log& operator<<(const T& v) {
		*(this->logger) << this->name << ": " << v;

		return *this;
	}
};
