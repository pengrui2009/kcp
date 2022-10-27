#include "timer.h"

#include <iostream>
#include <future>

/**
 * @brief Construct a new Timer:: Timer object
 * 
 * @param name 
 */
Timer::Timer(std::string name): name_(name), \
            expired_(true), try_to_expire_(false)
{

}

/**
 * @brief Construct a new Timer:: Timer object
 * 
 * @param timer 
 */
Timer::Timer(const Timer& timer)
{
    expired_ = timer.expired_.load();
    try_to_expire_ = timer.try_to_expire_.load();
}

/**
 * @brief Destroy the Timer:: Timer object
 * 
 */
Timer::~Timer()
{
    stop();
}

/**
 * @brief 
 * 
 * @param interval: unit 1ms
 * @param task : timer callback function
 */
void Timer::start(int interval, std::function<void()> task)
{
    // is started, do not start again
    if (expired_ == false)
        return;

    // start async timer, launch thread and wait in that thread
    expired_ = false;
    std::thread([this, interval, task]() {
        while (!try_to_expire_)
        {
            // sleep every interval and do the task again and again until times up
            std::unique_lock<std::mutex> locker(threadexpire_mutex__);
            if (thread_cond_.wait_for(locker, std::chrono::milliseconds(interval)) == std::cv_status::timeout)
            {
                task();
            }        
        
        }

        {
            // timer be stopped, update the condition variable expired and wake main thread
            std::lock_guard<std::mutex> locker(expire_mutex_);
            expired_ = true;
            expired_cond_.notify_one();
        }

    }).detach();
}

/**
 * @brief 
 * 
 * @param interval: unit:1ms.
 * @param task: callback function.
 */
void Timer::startOnce(int interval, std::function<void()> task)
{
    std::thread([this, interval, task]() {
        std::unique_lock<std::mutex> locker(threadexpire_mutex__);
        if (thread_cond_.wait_for(locker, std::chrono::milliseconds(interval)) == std::cv_status::timeout)
        {
            task();
        }
        {
            // timer be stopped, update the condition variable expired and wake main thread
            std::lock_guard<std::mutex> locker(expire_mutex_);
            expired_ = true;
            expired_cond_.notify_one();
        }
    }).detach();
}

/**
 * @brief 
 * 
 */
void Timer::stop(void)
{
    // do not stop again
    if (expired_)
        return;

    if (try_to_expire_)
        return;

    // wait until timer 
    try_to_expire_ = true; // change this bool value to make timer while loop stop
    {
        std::lock_guard<std::mutex> thread_locker(threadexpire_mutex__);

        thread_cond_.notify_one();
    }

    {
        std::unique_lock<std::mutex> locker(expire_mutex_);
        expired_cond_.wait(locker, [this] {return expired_ == true; });
        // reset the timer
        if (expired_ == true)
            try_to_expire_ = false;
    }
}

