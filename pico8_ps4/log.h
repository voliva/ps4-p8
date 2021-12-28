#pragma once

#include <string>
#include <vector>
#include <stdio.h>

#include "circular_buffer.h"

class Log;

class Logger
{
public:
	Logger();
	~Logger();

	Log log(std::string name);
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
	CircularBuffer<std::string> *buffer;
};

class Log
{
private:
	std::string name;
	Logger* logger;

public:
	Log(std::string name, Logger *logger);

	Log& operator<<(const char* v) {
		std::string string_value = std::string(v);
		*(this->logger) << (this->name + ": " + string_value).c_str();

		return *this;
	}
	template <class T>
	Log& operator<<(const T& v) {
		return *this << std::to_string(v).c_str();
	}
};
