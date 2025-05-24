#ifndef WEBSERVER_NET_TIMER_H
#define WEBSERVER_NET_TIMER_H
#include "../time/timeStamp.h"
#include <functional>
#include <assert.h>
namespace webserver{

class Timer{
    typedef std::function<void()> Func;
public:
    Timer(TimeStamp timeStamp, Func callback, double interval = -1.0):timeStamp_(timeStamp), callback_(callback), interval_(interval){}
    void run(){if(callback_){callback_();}}
    bool repeat(){return (interval_ >= 0.0);}
    void reset(){assert(repeat());timeStamp_.add(interval_*TimeStamp::kMicroSecondsPerSecond);}
    bool operator<(Timer timer){
        return this->timeStamp_ < timer.timeStamp_;
    }
private:
    TimeStamp timeStamp_;
    Func callback_;
    double interval_;
};






}



#endif