#pragma once

#include <string>
#include <mutex>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>

#include "circular_buffer.h"

class Log;

class Logger
{
public:
	Logger();
	~Logger();

	Log log(std::string name);
	std::thread listen_clients();
	int start_server();
	FILE *file_handle;

	Logger& operator<<(const char* v);
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
