#ifndef WEBSERVER_NET_CONNECTOR_H
#define WEBSERVER_NET_CONNECTOR_H

#include "channel.h"
#include "eventLoop.h"
#include "socket.h"

namespace webserver{

class Connector{
public:
    enum ConnState{
        sNew,
        sConnecting,
        sConnected,
        sDisconnected,
    };
    Connector(EventLoop* loop, InetAddress addr)
    :   loop_(loop), 
        fd_(sockets::createNonblockingSocket()), 
        addr_(addr),
        state_(sNew), 
        tryingConnect_(false), 
        channel_(new Channel(fd_, loop))
    {
        //channel_->setReadableCallback();
    }
    ~Connector(){
        close(fd_);
    }

    void stop(){
        tryingConnect_ = false;
    }

    void setNewConnectionCallback(function<void(int)> cb){
        newConnectionCallback_ = cb;
    }

    void tryConnect(){
        loop_->assertInLoopThread();
        tryingConnect_ = true;
        int ret = ::connect(fd_, (sockaddr*)addr_.getAddr(), sizeof(addr));
        if(ret == 0){
            newConnectionCallback_(fd_);
            tryIntervalMs_ = 500;
        }else{
            //错误处理。
            tryIntervalMs_ *= 2;
            retry();
        }
    }

    void closeCallback(){
        loop_->assertInLoopThread();
        if(tryingConnect_){
            retry();
        }else{
            ;
        }
    }

    void retry(){
        TimeStamp tryStamp = TimeStamp::now();
        tryStamp.add(tryIntervalMs_*1000);
        loop_->runAt(tryStamp, bind(tryConnect, this));
    }
private:
    EventLoop* loop_;
    function<void(int)> newConnectionCallback_;
    int fd_;
    unique_ptr<Channel> channel_;
    InetAddress addr_;
    
    ConnState state_;
    bool tryingConnect_;
    int tryIntervalMs_;


    

};

}
#endif