#ifndef MYNETLIB_COMMON_COUNTDOWNLATCH_H
#define MYNETLIB_COMMON_COUNTDOWNLATCH_H

#include "mutexLock.h"
#include "condition.h"
#include <pthread.h>

namespace mynetlib{

class CountDownLatch:Noncopyable{
public:
    CountDownLatch(int value):mutex_(), condition_(mutex_), counter_(value){
        pthread_cond_init(&cond_, NULL);
    }
    void countDown(){
        MutexLockGuard lock(mutex_);
        if(--counter_ == 0){
            condition_.notifyAll();
        }
    }
    void wait(){
        MutexLockGuard lock(mutex_);
        while(counter_ > 0){
            condition_.wait();
        }
    }
    ~CountDownLatch(){
        pthread_cond_destroy(&cond_);
    }
    // int getCount() const {
    //     MutexLockGuard lock(mutex_);
    //     return counter_;
    // }
private:
    MutexLock mutex_;
    Condition condition_ GUARDED_BY(mutex_);
    pthread_cond_t cond_;
    int counter_;
};

}







#endif