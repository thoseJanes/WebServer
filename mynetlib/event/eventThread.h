#ifndef MYNETLIB_NET_EVENTTHREAD_H
#define MYNETLIB_NET_EVENTTHREAD_H
#include "eventLoop.h"
#include "../../mynetbase/process/threadHandler.h"
#include "../../mynetbase/common/condition.h"
namespace mynetlib{

//假设start和析构在同一个线程中。仍有可能在loop之前quit。这个问题要如何解决？
class EventThread:Noncopyable{
public:
    typedef function<void(EventThread*)> ThreadInitCallback;
    EventThread(string_view name)
    :   thread_(bind(&EventThread::threadFunc, this), name), 
        mutex_(), 
        cond_(mutex_),
        loop_(NULL)
    {}

    void start(){
        thread_.start();
        {
            MutexLockGuard lock(mutex_);
            while(!loop_){
                cond_.wait();
            }
        }
    }

    EventLoop* getLoop(){
        MutexLockGuard lock(mutex_);
        return loop_;
    }

    string getName(){
        return thread_.getName();
    }
    
    void setThreadInitCallback(ThreadInitCallback func){
        assert(!thread_.started());
        thread_.assertInHandlerThread();
        // thread_.setInitThreadCallback(bind(func, loop_));
        threadInitCallback_ = func;
    }

    ~EventThread(){
        join();
    }

    void join(){
        if(loop_){
            loop_->quit();
            thread_.join();
        }
    }
private:
    void threadFunc(){
        EventLoop loop;
        {
            MutexLockGuard lock(mutex_);
            loop_ = &loop;
            cond_.notify();
        }
        
        if(threadInitCallback_){
            threadInitCallback_(this);
        }
        loop.loop();

        {
            MutexLockGuard lock(mutex_);
            loop_ = NULL;
        }
    }

    ThreadHandler thread_;
    MutexLock mutex_;
    Condition cond_;
    EventLoop* loop_;
    ThreadInitCallback threadInitCallback_;
    //function<void()> threadInitCallback_;
};

}

#endif