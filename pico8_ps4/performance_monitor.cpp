#include "performance_monitor.h"
#include "log.h"


#define DEBUGLOG Performance_DEBUGLOG
Log DEBUGLOG = logger.log("Performance");

void PerformanceMonitor::registerStart(std::string task)
{
	if (this->tasks.count(task) == 0) {
		this->tasks[task] = Task{
			std::chrono::high_resolution_clock::now(),
			0,
			0,
			0,
			0x7FFFFFFFFFFFFFFF
		};
	}
	else {
		this->tasks[task].last_start = std::chrono::high_resolution_clock::now();
	}
}

void PerformanceMonitor::registerEnd(std::string task)
{
	if (this->tasks.count(task) == 0) {
		return;
	}
	auto now = std::chrono::high_resolution_clock::now();
	auto timediff = std::chrono::duration_cast<std::chrono::nanoseconds>(now - this->tasks[task].last_start).count();
	this->tasks[task].total_time += timediff;
	this->tasks[task].total_calls++;
	this->tasks[task].longest_call = std::max(this->tasks[task].longest_call, timediff);
	this->tasks[task].shortest_call = std::min(this->tasks[task].shortest_call, timediff);
}

void PerformanceMonitor::logStats()
{
	DEBUGLOG << "PERFORMANCE REPORT" << ENDL;
	for (std::map<std::string, Task>::iterator it = this->tasks.begin(); it != this->tasks.end(); it++) {
		DEBUGLOG << it->first << ": total = " << it->second.total_time << " / " << it->second.total_calls << ", "
			<< "longest: " << it->second.longest_call << ", shortest: " << it->second.shortest_call << ENDL;
	}
}

void PerformanceMonitor::reset()
{
	this->tasks.clear();
}
