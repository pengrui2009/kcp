#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <string>
#include <functional>
#include <cstdint>
#include <chrono>
#include <thread>

class Timestamp
{
public:
    Timestamp()
        : begin_time_point_(std::chrono::high_resolution_clock::now())
    { }

    void Reset()
    {
        begin_time_point_ = std::chrono::high_resolution_clock::now();
    }

    int64_t Elapsed()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - begin_time_point_).count();
    }

    static std::string Now();
    static uint64_t Second();
    static uint64_t MillSecond();
    static uint64_t MicroSecond();
    static uint64_t NanoSecond();
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> begin_time_point_;
};

#endif