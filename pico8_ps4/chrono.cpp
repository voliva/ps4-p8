#include "chrono.h"

timestamp_t getTimestamp()
{
#ifdef __PS4__
    return sceKernelGetProcessTime();
#else
    return std::chrono::high_resolution_clock::now();
#endif
}

timestamp_t nilTimestamp()
{
#ifdef __PS4__
    return 0;
#else
    return std::chrono::time_point<std::chrono::high_resolution_clock>();
#endif
}

bool isTimestampNil(timestamp_t t)
{
    return getMicrosecondsDiff(t, nilTimestamp()) == 0;
}

long long getMicrosecondsDiff(timestamp_t t1, timestamp_t t2)
{
#ifdef __PS4__
    return t1 - t2;
#else
    return std::chrono::duration_cast<std::chrono::microseconds>(t1 - t2).count();
#endif
}

long long getMillisecondsDiff(timestamp_t t1, timestamp_t t2)
{
    return getMicrosecondsDiff(t1, t2) / 1000;
}

