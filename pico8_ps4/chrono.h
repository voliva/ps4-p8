#ifdef __PS4__
#include <orbis/libkernel.h>

typedef uint64_t timestamp_t;
#else
#include <chrono>

typedef std::chrono::system_clock::time_point timestamp_t;
#endif

timestamp_t getTimestamp();
timestamp_t nilTimestamp();
bool isTimestampNil(timestamp_t t);
long long getMicrosecondsDiff(timestamp_t t1, timestamp_t t2);
long long getMillisecondsDiff(timestamp_t t1, timestamp_t t2);
