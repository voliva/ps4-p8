#include "log.h"
#include "file_paths.h"
#include <thread>
#include <set>

Logger logger;

Logger::Logger()
{
	this->file_handle = NULL;
}

Logger::~Logger()
{
	if (this->file_handle != NULL)
		fclose(this->file_handle);
}

Logger& Logger::operator<<(const char* v)
{
	if (this->file_handle == NULL) {
		this->file_handle = fopen(FILENAME, "w");
	}
	if (this->file_handle == NULL) return *this;

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
