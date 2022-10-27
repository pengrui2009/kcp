
#ifndef TIMER_H
#define TIMER_H

#include <thread>
#include <chrono>

#include <atomic>
#include <mutex>
#include <condition_variable>

#include <memory>

#include <functional>


enum TIMER_TYPE_EM
{
    TIMER_ONESHOT = 0,
    TIMER_INTERVAL = 1
};

enum TIMER_MODE_EM
{
    TIMER_SYNC = 0,
    TIMER_ASYNC = 1
};

#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>

class Timer
{
public:
    /******************************************************************************
     *    Function: Timer
     *    Descriptions: constructor function 
     *    Paramters:
                name                  - the name of timer
     *    Return:
                none
     *    Comments:
    ******************************************************************************/
    Timer(std::string name);
    /******************************************************************************
     *    Function: Timer
     *    Descriptions:copy constructor function 
     *    Paramters:
                timer                  - the timer of this
     *    Return:
                none
     *    Comments:
    ******************************************************************************/
	Timer(const Timer& timer);
    /******************************************************************************
     *    Function: Timer
     *    Descriptions: deconstructor function 
     *    Paramters:
                name                  - the name of timer
     *    Return:
                none
     *    Comments:
    ******************************************************************************/
	~Timer();
    /******************************************************************************
     *    Function: start
     *    Descriptions: start timer with interval loop
     *    Paramters:
                interval                  - the period of time (unit:1ms)
                task                      - the callback of task.
     *    Return:
                void
     *    Comments:
    ******************************************************************************/
	void start(int interval, std::function<void()> task);
	
    /******************************************************************************
     *    Function: startOnce
     *    Descriptions: start timer with once loop
     *    Paramters:
                interval                  - the period of time (unit:1ms)
                task                      - the callback of task.
     *    Return:
                void
     *    Comments:
    ******************************************************************************/
	void startOnce(int interval, std::function<void()> task);
    /******************************************************************************
     *    Function: stop
     *    Descriptions: stop timer
     *    Paramters:
                void
     *    Return:
                void
     *    Comments: will wati until all task finished.
    ******************************************************************************/
	void stop(void);

private:
    // the name of timer.
    std::string name_;

	std::atomic<bool> expired_; // timer stopped status
	std::atomic<bool> try_to_expire_; // timer is in stop process
	std::mutex expire_mutex_;
	std::condition_variable expired_cond_;

    std::mutex threadexpire_mutex__;
    std::condition_variable_any thread_cond_;
};

#endif /* TIMER_H */