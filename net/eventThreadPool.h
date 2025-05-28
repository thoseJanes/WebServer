#ifndef WEBSERVER_NET_EVENTTHREADPOOL_H
#define WEBSERVER_NET_EVENTTHREADPOOL_H
#include "eventThread.h"
namespace webserver{


class EventThreadPool{
public:
    EventThreadPool(EventLoop* baseLoop, int threadNum):next_(0){
        baseLoop_.reset(baseLoop);
    }

    void start(function<void()> threadInitCallback){
        for(int i=0;i<threadNum_;i++){
            eventThreads_.emplace_back(new EventThread());
            eventThreads_[i]->setThreadInitCallback(threadInitCallback);
            eventThreads_[i]->start();
        }
    }
    EventLoop* getNextLoop(){
        baseLoop_->assertInLoopThread();
        if(eventThreads_.empty()){
            return baseLoop_.get();
        }else{
            return eventThreads_[next_++]->getLoop();
        }
    }
    EventLoop* getEventLoopByHash(int hashCode){
        baseLoop_->assertInLoopThread();
        if(eventThreads_.empty()){
            return baseLoop_.get();
        }else{
            return eventThreads_[hashCode%eventThreads_.size()]->getLoop();
        }
    }

    ~EventThreadPool();
    
private:
    int threadNum_;
    int next_;
    std::vector<unique_ptr<EventThread>> eventThreads_;
    unique_ptr<EventLoop> baseLoop_;
};


}

#endif