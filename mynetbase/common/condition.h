#ifndef MYNETLIB_COMMON_CONDITION_H
#define MYNETLIB_COMMON_CONDITION_H
#include "mutexLock.h"
#include <pthread.h>
#include <errno.h>

namespace mynetlib{


class Condition:Noncopyable{
public:
    Condition(MutexLock& mutex):mutex_(mutex){
        MCHECK(pthread_cond_init(&cond_, NULL));
    }

    void wait(){
        BlockGuard block(mutex_);
        MCHECK(pthread_cond_wait(&cond_, mutex_.pthreadMutex()));
    }

    void notify(){
        MCHECK(pthread_cond_signal(&cond_));
    }

    void notifyAll(){
        MCHECK(pthread_cond_broadcast(&cond_));
    }

    bool waitSeconds(int seconds){
        // timespec interval;
        // interval.tv_sec = seconds;
        // interval.tv_nsec = 0;
        // BlockGuard block(mutex_);
        // return ETIMEDOUT == pthread_cond_timedwait(&cond_, mutex_.pthreadMutex(), &interval);
        return waitMilliseconds(seconds*1000);
    }

    bool waitMilliseconds(int ms){
        timespec interval;
        interval.tv_sec = ms/1000;
        interval.tv_nsec = ms%1000*1000;
        BlockGuard block(mutex_);
        return ETIMEDOUT == pthread_cond_timedwait(&cond_, mutex_.pthreadMutex(), &interval);
    }

    ~Condition(){
        MCHECK(pthread_cond_destroy(&cond_));
    }
private:
    pthread_cond_t cond_;
    MutexLock& mutex_;
};


}


#endif