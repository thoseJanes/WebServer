#ifndef WEBSERVER_COMMON_MUTEX_H
#define WEBSERVER_COMMON_MUTEX_H

#include <pthread.h>
#include <assert.h>
#include "patterns.h"

namespace webserver{

#define MCHECK(ret) \
({ \
    __typeof__(ret) errnum = (ret); \
    assert(errnum == 0); \
    (void)errnum; \
})

class Mutex:Noncopyable{
public:
    Mutex(){MCHECK(pthread_mutex_init(&mutex_, NULL));}
    void lock(){MCHECK(pthread_mutex_lock(&mutex_));}
    void unlock(){MCHECK(pthread_mutex_unlock(&mutex_));}
    ~Mutex(){MCHECK(pthread_mutex_destroy(&mutex_));}

private:
    pthread_mutex_t mutex_;

};

class MutexGuard:Noncopyable{
public:
    MutexGuard(Mutex& mutex):mutex_(mutex){
        mutex_.lock();
    }
    ~MutexGuard(){
        mutex_.unlock();
    }
private:
    Mutex& mutex_;
};

#define MutexGuard(mutex) error "create MutexGuard without object name."

}

#endif