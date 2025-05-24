#ifndef WEBSERVER_NET_CHANNEL_H
#define WEBSERVER_NET_CHANNEL_H

#include <poll.h>
namespace webserver{

class EventLoop;
class Channel{
public:
    enum PollState{
        None,
        Active,
        Disabled,
        Deleted,
    };
    Channel(int fd, EventLoop* loop):fd_(fd), loop_(loop), state_(None), event_(0), revent_(0){}
    void update();

    void enableReadableEvent(function<void()> func){
        event_ |= POLLIN; readableCallback_ = std::move(func);
    }
    void enableWritableEvent(function<void()> func){
        event_ |= POLLOUT; writableCallback_ = std::move(func);
    }


    void run(){
        
    }


    void clearEvent(){
        event_ = 0;
    }
    bool hasEvent(){
        return event_ != 0;
    }

    void assertInLoopThread();

    ~Channel() = default;
    
private:
    int fd_;
    EventLoop* loop_;
    
    int event_;
    int revent_;//由poller填入

    typedef function<void()> Callback;
    Callback readableCallback_;
    Callback writableCallback_;
    Callback closeCallback_;


    PollState state_;
};

}
#endif