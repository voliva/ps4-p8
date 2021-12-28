#include "log.h"
#include <thread>

#ifdef __PS4__
#define FILENAME "/data/debug.log"
#else
#define FILENAME "debug.log"
#endif

Logger::Logger()
{
	this->buffer = new CircularBuffer<std::string>(100);
	this->file_handle = fopen(FILENAME, "w");
}

Logger::~Logger()
{
	delete this->buffer;
	fclose(this->file_handle);
}

Logger& Logger::operator<<(const char* v)
{
	printf("%s\n", v);

	this->buffer->push(v);
	fputs(v, this->file_handle);
	fputs("\n", this->file_handle);
	fflush(this->file_handle);

	return *this;
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
