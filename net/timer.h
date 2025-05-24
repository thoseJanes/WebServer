#ifndef WEBSERVER_NET_TIMER_H
#define WEBSERVER_NET_TIMER_H
#include "../time/timeStamp.h"
#include "../logging/logger.h"
#include <functional>
#include <assert.h>
#include <atomic>
#include <limits>
namespace webserver{

class Timer{
    typedef std::function<void()> Func;
public:
    Timer(TimeStamp timeStamp, Func callback, double interval = -1.0):timeStamp_(timeStamp), callback_(callback), interval_(interval){
        if(__builtin_expect(id == std::numeric_limits<int>::max(), 0)){
            LOG_FATAL << "Timer::id overflow";
        }
        id.fetch_add(1);
        id_ = id;
    }
    void run(){if(callback_){callback_();}}
    bool isRepeat(){return (interval_ >= 0.0);}
    void reset(){assert(isRepeat());timeStamp_.add(interval_*TimeStamp::kMicroSecondsPerSecond);}
    bool operator<(Timer timer){
        return this->timeStamp_ < timer.timeStamp_;
    }
    const TimeStamp& getTimeStamp(){return timeStamp_;}
    const int getTimerId(){return id_;}
    
private:
    TimeStamp timeStamp_;
    Func callback_;
    double interval_;
    int id_;
    static std::atomic<int> id;
    
};


struct TimerId{
    int id;
    Timer* timer;
};



}



#endif