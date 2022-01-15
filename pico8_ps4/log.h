#pragma once

#include <string>
#include <vector>
#include <stdio.h>

class Log;

#define ENDL "\n"

class Logger
{
public:
	Logger();
	~Logger();

	Log log(std::string name);
	FILE *file_handle;

	Logger& operator<<(const char* v);
	Logger& operator<<(std::string v);
	template <class T>
	Logger& operator<<(const T& v) {
		return *this << std::to_string(v);
	}
};

extern Logger logger;

class Log
{
private:
	std::string name;
	Logger* logger;
	bool appended_label;

public:
	Log(std::string name, Logger *logger);

	Log& operator<<(std::string v) {
		if (!this->appended_label) {
			*(this->logger) << this->name << ": ";
			this->appended_label = true;
		}
		*(this->logger) << v;

		if (v.find("\n") != std::string::npos) {
			this->appended_label = false;
		}

		return *this;
	}
	Log& operator<<(const char* v) {
		return *this << std::string(v);
	}
	template <class T>
	Log& operator<<(const T& v) {
		return *this << std::to_string(v);
	}
};
