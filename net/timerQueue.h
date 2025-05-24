#ifndef WEBSERVER_NET_TIMERQUEUE_H
#define WEBSERVER_NET_TIMERQUEUE_H
#include "timer.h"

#include <set>
#include <sys/timerfd.h>
namespace webserver{

namespace detail{

int createTimerFd(){
    int fd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC | TFD_NONBLOCK | TFD_TIMER_ABSTIME);
    
}

int readTimerFd(){

}

int setTimerFd(TimeStamp timeStamp){

}

}

class TimeQueue{
public:
    TimeQueue();
    ~TimeQueue();
    void addTimer(Timer timer){
        
    }

private:
    std::set<Timer, std::less<Timer>> timers_;//通过TimeStamp比较的Timer？将最小的放在最上面。
};

}

#endif