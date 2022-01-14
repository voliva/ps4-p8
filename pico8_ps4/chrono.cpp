#include "chrono.h"

timestamp_t getTimestamp()
{
#ifdef __PS4__
    return sceKernelGetProcessTime();
#else
    return std::chrono::high_resolution_clock::now();
#endif
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
