#include "log.h"
#include <thread>

#ifdef __PS4__
#define FILENAME "/data/debug.log"
#else
#define FILENAME "debug.log"
#endif

Logger logger;

Logger::Logger()
{
	this->file_handle = fopen(FILENAME, "w");
}

Logger::~Logger()
{
	fclose(this->file_handle);
}

Logger& Logger::operator<<(const char* v)
{
	printf("%s", v);

	fputs(v, this->file_handle);
	fflush(this->file_handle);

	return *this;
}

Logger& Logger::operator<<(std::string v)
{
	return *this << v.c_str();
}

Log Logger::log(std::string name)
{
	return Log(name, this);
}

Log::Log(std::string name, Logger *logger)
{
	this->name = name;
	this->logger = logger;
	setvbuf(stdout, NULL, _IONBF, 0);
}
