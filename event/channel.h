#ifndef WEBSERVER_NET_CHANNEL_H
#define WEBSERVER_NET_CHANNEL_H

#include <poll.h>
#include <assert.h>
#include <any>
#include <functional>
#include <memory>
#include "../logging/logger.h"

namespace webserver{

using namespace std;
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
    :fd_(fd), loop_(loop), state_(sNone), event_(kNoneEvent), revent_(kNoneEvent), tied_(false), handlingEvents_(false){}
    ~Channel();
    void update();
    void remove();

    void setReadableCallback(const function<void()> func){readableCallback_ = std::move(func);}
    void setWritableCallback(const function<void()> func){writableCallback_ = std::move(func);}
    void setErrorCallback(const function<void()> func){errorCallback_ = std::move(func);}
    void setCloseCallback(const function<void()> func){closeCallback_ = std::move(func);}
    void enableReading(){event_ |= kReadingEvent; update();}
    void enableWriting(){event_ |= kWritingEvent; update();}
    void disableReading(){event_ &= ~kReadingEvent; update();}
    void disableWriting(){event_ &= ~kWritingEvent; update();}
    void disableAll(){event_ &= kNoneEvent; update();}
    bool isReadingEnabled(){return (event_ & kReadingEvent);}
    bool isWritingEnabled(){return (event_ & kWritingEvent);}

    void handleEvent(){
        if(tied_){
            auto tieGuard = make_shared<std::any>(tie_);
            if(tieGuard){
                run();
            }else{
                LOG_SYSERROR << "Failed in handleEvent.";
            }
        }else{
            run();
        }
    }
    void run(){
        handlingEvents_ = true;
        if(tied_){
            shared_ptr<void> guard = tie_.lock();
            runWithGuard();
        }else{
            runWithGuard();
        }
        handlingEvents_ = false;
    }
    void runWithGuard(){
        if(revent_&POLLHUP && !(revent_&POLLIN)){
            closeCallback_();
        }

        // if(revent_ & (POLLIN|POLLPRI)){
        //     readableCallback_();
        // }else if(revent_ & (POLLOUT)){
        //     writableCallback_();
        // }else if(revent_ & (POLLERR|POLLNVAL)){
        //     errorCallback_();
        // }
        if(revent_ & (POLLERR|POLLNVAL)){
            if(errorCallback_) errorCallback_();
        }
        if(event_ & (POLLIN | POLLPRI | POLLRDHUP)){
            if(readableCallback_) readableCallback_();
        }
        if(event_ & (POLLOUT)){
            if(writableCallback_) writableCallback_();
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

    void tie(const shared_ptr<void> t){
        tie_ = t;
        tied_ = true;
    }
    void untie(){
        tie_.reset();
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
    bool handlingEvents_;
    weak_ptr<void>tie_;
    bool tied_;

    static int kReadingEvent;
    static int kWritingEvent;
    static int kNoneEvent;
};

}
#endif