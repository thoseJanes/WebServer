#ifndef WEBSERVER_NET_EVENTTHREAD_H
#define WEBSERVER_NET_EVENTTHREAD_H
#include "eventLoop.h"
#include "../process/threadHandler.h"
#include "../common/condition.h"
namespace webserver{

//假设start和析构在同一个线程中。仍有可能在loop之前quit。这个问题要如何解决？
class EventThread{
public:
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
    
    void setThreadInitCallback(function<void()> func){
        assert(!thread_.started());
        thread_.assertInHandlerThread();
        //threadInitCallback_ = func;
        thread_.setInitThreadCallback(func);
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
    //function<void()> threadInitCallback_;
};

}

#endif