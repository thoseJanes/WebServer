#ifndef MYNETLIB_NET_EVENTTHREADPOOL_H
#define MYNETLIB_NET_EVENTTHREADPOOL_H
#include "eventThread.h"
namespace mynetlib{


class EventThreadPool:Noncopyable{
public:
    EventThreadPool(EventLoop* baseLoop, string_view name):next_(0), baseLoop_(baseLoop), name_(name){
    }

    void start(int threadNum, EventThread::ThreadInitCallback threadInitCallback){
        assert(threadNum >= 0);
        char buf[name_.size()+std::numeric_limits<int>::max_digits10+12];
        for(int i=0;i<threadNum;i++){
            int len = snprintf(buf, sizeof(buf), "%s-thread%d", name_.c_str(), i);
            eventThreads_.emplace_back(new EventThread(string_view(buf, len-1)));
            eventThreads_[i]->setThreadInitCallback(threadInitCallback);
            eventThreads_[i]->start();
        }
    }
    EventLoop* getNextLoop(){
        baseLoop_->assertInLoopThread();
        if(eventThreads_.empty()){
            return baseLoop_;
        }else{
            if(next_ >= eventThreads_.size()){
                next_ = 0;
            }
            return eventThreads_[next_++]->getLoop();
        }
    }
    EventLoop* getEventLoopByHash(int hashCode){
        baseLoop_->assertInLoopThread();
        if(eventThreads_.empty()){
            return baseLoop_;
        }else{
            return eventThreads_[hashCode%eventThreads_.size()]->getLoop();
        }
    }

    ~EventThreadPool(){
        for(unique_ptr<EventThread>& eventThread:eventThreads_){
            eventThread->join();
        }
    }
    
private:
    //int threadNum_;
    int next_;
    std::vector<unique_ptr<EventThread>> eventThreads_;
    EventLoop* baseLoop_;
    string name_;
};


}

#endif