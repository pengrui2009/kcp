#include "timestamp.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace std;
using namespace std::chrono;

std::string Timestamp::Now()
{
    std::ostringstream stream;
    auto now = system_clock::now();
    time_t tt = system_clock::to_time_t(now);
	
#if defined(WIN32) || defined(_WIN32)
    struct tm tm;
    localtime_s(&tm, &tt);
    stream << std::put_time(&tm, "%F %T");
#elif  defined(__linux) || defined(__linux__) 
    char buffer[200] = {0};
    std::string timeString;
    std::strftime(buffer, 200, "%F %T", std::localtime(&tt));
    stream << buffer;
#endif	
    return stream.str();
}

uint64_t Timestamp::Second()
{
    std::ostringstream stream;
    auto now = system_clock::now();
    time_t tt = system_clock::to_time_t(now);
	
    return static_cast<uint64_t>(tt);
}

uint64_t Timestamp::MillSecond()
{    
    uint64_t value = 0;
    
    uint32_t second = 0;
    std::chrono::milliseconds millsecond;
    std::chrono::microseconds micrsecond;
    std::chrono::nanoseconds nanosecond;

    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    second = now.time_since_epoch().count() * system_clock::period::num / system_clock::period::den;
    millsecond = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    // micrsecond = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
    // nanosecond = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()) % 1000000000;
    value = static_cast<uint64_t>(second) * 1000 + millsecond.count();

    return value;
}

uint64_t Timestamp::MicroSecond()
{
    uint64_t value = 0;

    uint32_t second = 0;
    // std::chrono::milliseconds millsecond;
    std::chrono::microseconds microsecond;
    // std::chrono::nanoseconds nanosecond;

    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    second = now.time_since_epoch().count() * system_clock::period::num / system_clock::period::den;
    // millsecond = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    microsecond = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
    // nanosecond = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()) % 1000000000;
    value = static_cast<uint64_t>(second) * 1000000 + microsecond.count();

    return value;
}

uint64_t Timestamp::NanoSecond()
{
    uint64_t value = 0;

    uint32_t second = 0;
    // std::chrono::milliseconds millsecond;
    // std::chrono::microseconds micrsecond;
    std::chrono::nanoseconds nanosecond;

    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    second = now.time_since_epoch().count() * system_clock::period::num / system_clock::period::den;
    // millsecond = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    // micrsecond = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
    nanosecond = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()) % 1000000000;
    value = static_cast<uint64_t>(second) * 1000000000 + nanosecond.count();

    return value;
}