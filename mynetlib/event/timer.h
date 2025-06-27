#ifndef MYNETLIB_NET_TIMER_H
#define MYNETLIB_NET_TIMER_H
#include "../../mybase/time/timeStamp.h"
#include "../../mybase/logging/logger.h"
#include <functional>
#include <assert.h>
#include <atomic>
#include <limits>
namespace mynetlib{

class Timer:Noncopyable{
    typedef std::function<void()> Func;
public:
    explicit Timer(TimeStamp timeStamp, Func callback, double interval = -1.0):timeStamp_(timeStamp), callback_(callback), interval_(interval){
        if(__builtin_expect(id == std::numeric_limits<int>::max(), 0)){
            LOG_FATAL << "Timer::id overflow";
        }
        id.fetch_add(1);
        id_ = id;
    }
    void run(){if(callback_){callback_();}}
    bool isRepeat(){return (interval_ > 0.0);}
    //void restart(){assert(isRepeat());timeStamp_.add(interval_*TimeStamp::kMicroSecondsPerSecond);}
    void toNextTime(){assert(isRepeat());timeStamp_.add(interval_*TimeStamp::kMicroSecondsPerSecond);}
    const TimeStamp& getTimeStamp(){return timeStamp_;}
    const int getTimerId(){return id_;}
    
private:
    TimeStamp timeStamp_;
    Func callback_;
    double interval_;
    int id_;
    static std::atomic<int> id;
    
};


class TimerId{
public:
    explicit TimerId(int id):timerId_(id){};
    TimerId():timerId_(-1){};
    int id(){return timerId_;}
    int valid(){return timerId_>=0;}
private:
    int timerId_;
};



}



#endif