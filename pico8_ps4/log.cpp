#include "log.h"
#include <thread>
#include <set>

#ifdef __PS4__
#define FILENAME "/data/debug.log"
#elif __SWITCH__
#define FILENAME "/debug.log"
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


std::set<std::string> alerted;
void alert_todo(std::string key) {
	if (alerted.count(key) == 0) {
		logger << "TODO called: " << key << ENDL;
		alerted.insert(key);
	}
}
