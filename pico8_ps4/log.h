#pragma once

#include <string>
#include <mutex>
#include <vector>
#include <unistd.h>

class Log;

class Logger
{
public:
	Logger();

	Log log(std::string name);

	Logger& operator<<(const char* v) {
		printf("%s", v);

		this->mtx.lock();

		if (this->active_client > 0) {
			write(this->active_client, v, sizeof(v));
		}
		else {
			this->buffer.push_back(v);
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
	std::vector<std::string> buffer;
	int active_client;
	int start_server();
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
