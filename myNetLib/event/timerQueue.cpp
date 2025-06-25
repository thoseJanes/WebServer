#include "timerQueue.h"

using namespace mynetlib;

int detail::createTimerFd(){
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);// | TFD_TIMER_ABSTIME);
    if(fd < 0){
        LOG_FATAL << "failed in createTimerFd";
    }
    return fd;
}

void detail::readTimerFd(int fd){
    int64_t ovt;
    int n = ::read(fd, &ovt, sizeof(ovt));
    if(n != 8){
        LOG_SYSERROR << "expect 8 bytes but read " << n << " bytes in readTimerFd()!";
    }
}

void detail::setTimerFd(int fd, TimeStamp timeStamp){//setTimerfd时，是否需要取消之前设置的timerfd？
    int64_t microSeconds = timeStamp.getMicroSecondsSinceEpoch() - TimeStamp::now().getMicroSecondsSinceEpoch();
    if(microSeconds < 0){
        LOG_WARN << "microSeconds "<< microSeconds <<" is negative in setTimerFd()!";
    }

    if(microSeconds < 100){
        microSeconds = 100;
    }

    itimerspec t; memset(&t, 0, sizeof(t));
    t.it_value.tv_sec = microSeconds/TimeStamp::kMicroSecondsPerSecond;
    t.it_value.tv_nsec = ((microSeconds%TimeStamp::kMicroSecondsPerSecond)) * 1000;//微秒到纳秒的转换。以及分辨率0.1毫秒。
    //原项目不需要规整，只是在microSeconds小于100时设置其为100,即设置了一个最小界限。
    itimerspec out;
    LOG_TRACE<< "set timerfd at "<< timeStamp.toFormattedString(true) <<" with microsec offset "<< microSeconds <<" to second "<<t.it_value.tv_sec<<" and nsec "<<t.it_value.tv_nsec;
    if(timerfd_settime(fd, 0, &t, &out)){//不会吧，设置成0了？这怎么办？
        LOG_FATAL << "failed in timerfd_settime().";
    }
}