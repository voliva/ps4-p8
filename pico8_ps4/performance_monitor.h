#pragma once

#include <map>
#include <chrono>
#include <string>

typedef struct {
	std::chrono::steady_clock::time_point last_start;
	unsigned long long total_time;
	int total_calls;
	long long longest_call;
	long long shortest_call;
} Task;

class PerformanceMonitor
{
public:
	void registerStart(std::string task);
	void registerEnd(std::string task);
	void logStats();
	void reset();

private:
	std::map<std::string, Task> tasks;
};
extern PerformanceMonitor* performanceMonitor;
