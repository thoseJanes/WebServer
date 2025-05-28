#ifndef WEBSERVER_NET_CHANNEL_H
#define WEBSERVER_NET_CHANNEL_H

#include <poll.h>
#include <assert.h>
#include <any>
#include "../logging/logger.h"
namespace webserver{

class EventLoop;
class Channel{
public:
    enum PollState{
        sNone,
        sActive,
        sDisabled,
        sDeleted,
    };
    Channel(int fd, EventLoop* loop)
    :fd_(fd), loop_(loop), state_(sNone), event_(kNoneEvent), revent_(kNoneEvent), tied_(false){}
    ~Channel(){
        assert(state_ == sNone || state_ == sDeleted);//为什么不在析构函数中将channelremove出poller？
    }
    void update();
    void remove();

    void setReadableCallback(function<void()> func){readableCallback_ = func;}
    void setWritableCallback(function<void()> func){writableCallback_ = func;}
    void setErrorCallback(function<void()> func){errorCallback_ = func;}
    void setCloseCallback(function<void()> func){closeCallback_ = func;}
    void enableReading(){event_ |= kReadingEvent; update();}
    void enableWritting(){event_ |= kWrittingEvent; update();}
    void disableAll(){event_ &= kNoneEvent; update();}
    bool isReadingEnabled(){return (event_ & kReadingEvent);}
    bool isWrittingEnabled(){return (event_ & kWrittingEvent);}

    void handleEvent(){
        if(tied_){
            auto tieGuard = make_shared<std::any>(tie_);
            if(tieGuard){
                run();
            }else{
                LOG_ERR << "Failed in handleEvent.";
            }
        }else{
            run();
        }
    }
    void run(){
        if(revent_&POLLHUP && !(revent_&POLLIN)){
            closeCallback_();
        }

        if(revent_ & (POLLIN|POLLPRI)){
            readableCallback_();
        }else if(revent_ & (POLLOUT)){
            writableCallback_();
        }else if(revent_ & (POLLERR|POLLNVAL)){
            errorCallback_();
        }
    }

    void assertInLoopThread();
    
    int getPollState(){return state_;}
    void setPollState(PollState state){state_ = state;}
    int getFd(){return fd_;}
    int getEvent(){return event_;}
    bool hasEvent(){
        return event_ != 0;
    }
    void setREvent(int revent){
        revent_ = revent;
    }

    void tie(weak_ptr<std::any> t){
        tie_ = t.lock();
        tied_ = true;
    }
    void untie(){
        tie_ = NULL;
        tied_ = false;
    }
private:
    int fd_;
    EventLoop* loop_;
    
    int event_;
    int revent_;//由poller填入

    typedef function<void()> Callback;
    Callback readableCallback_;
    Callback writableCallback_;
    Callback closeCallback_;
    Callback errorCallback_;

    PollState state_;
    std::any tie_;
    bool tied_;

    static int kReadingEvent;
    static int kWrittingEvent;
    static int kNoneEvent;
};

}
#endif