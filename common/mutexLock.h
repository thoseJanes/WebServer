#ifndef WEBSERVER_COMMON_MUTEX_H
#define WEBSERVER_COMMON_MUTEX_H

#include <pthread.h>
#include <assert.h>
#include "patterns.h"
#include "../process/currentThread.h"
#include "threadSafetyAnalysis.h"

namespace webserver{

#define MCHECK(ret) \
({ \
    __typeof__(ret) errnum = (ret); \
    assert(errnum == 0); \
    (void)errnum; \
})

class CAPABILITY("mutex") MutexLock:Noncopyable{
public:
    MutexLock(){MCHECK(pthread_mutex_init(&mutex_, NULL));}
    void lock() ACQUIRE() {
        MCHECK(pthread_mutex_lock(&mutex_));
        assignHolder();
    }
    void unlock() RELEASE() {
        unassignHolder();
        MCHECK(pthread_mutex_unlock(&mutex_));
    }
    ~MutexLock(){MCHECK(pthread_mutex_destroy(&mutex_));}
    bool lockedInPresentThread(){
        return holder_ == CurrentThread::tid();
    }
    bool isLocked(){
        return holder_ != 0;
    }
private:
    void assignHolder(){
        holder_ = CurrentThread::tid();
    }
    void unassignHolder(){
        holder_ = 0;
    }
    pthread_mutex_t* pthreadMutex(){return &mutex_;}

    friend class Condition;
    friend class BlockGuard;
    friend class CountDownLatch;
    pthread_mutex_t mutex_;
    pid_t holder_ = 0;
};


class BlockGuard{
public:
    BlockGuard(MutexLock& owner):owner_(owner){
        owner_.unassignHolder();
    }
    ~BlockGuard(){
        owner_.assignHolder();
    }
private:
    MutexLock& owner_;
};


class SCOPED_CAPABILITY MutexLockGuard:Noncopyable{
public:
    MutexLockGuard(MutexLock& mutex) ACQUIRE(mutex) :mutex_(mutex){//这里必须显式地获取mutex，而非mutex_
        mutex_.lock();
    }
    ~MutexLockGuard() RELEASE() {
        mutex_.unlock();
    }
private:
    MutexLock& mutex_;
};

#define MutexLockGuard(mutex) error "create MutexGuard without object name."
#define BlockGuard(mutex) error "create BlockGuard without object name."

}

#endif