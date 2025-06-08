#ifndef WEBSERVER_NET_CONNECTOR_H
#define WEBSERVER_NET_CONNECTOR_H

#include "../event/channel.h"
#include "../event/eventLoop.h"
#include "socket.h"
// #include <atomic>
#include <cstdlib>

namespace webserver{

//即使多次调用start，有可能会在某一时间出现多个retry，但是没关系，至少不会发生致命错误。
//注意实现方式，这是通过设置状态ConnState实现的。
class Connector: public enable_shared_from_this<Connector>{
public:
    enum ConnState{
        sConnected,
        sConnecting,
        sWaitRetry,
        sDisconnected,
    };
    Connector(EventLoop* loop, InetAddress addr)
    :   loop_(loop), 
        addr_(addr),
        state_(sDisconnected), 
        tryingConnect_(false),
        tryIntervalMs_(500)
    {
        //channel_->setReadableCallback();
    }
    ~Connector(){
        tryingConnect_ = false;
        // if(channel_){
        //     if(channel_->isWritingEnabled()){
        //         channel_->disableAll();
        //         channel_->remove();
        //     }
        //     channel_.reset();//这貌似也不安全？如果刚好在执行channel呢？为啥这些东西好像都不安全？connector和它的channel也不是在同一个loop里吧？
        //     //需要放进pendingFunctors中析构吗？当然需要先删除定时器？
        // }
        assert(!channel_);
        resetTiemrId(true);
        //貌似还是需要删除定时器，怕析构之后执行回调发生未定义行为。
        //或者直接assert，由上层保证？
    }

    //貌似可能与loop线程内的reset操作冲突？？
    void resetTiemrId(bool cancelTimer){
        if(retryTimerId_ -> valid()){
            if(cancelTimer){
                loop_->cancelTimer(*retryTimerId_);
            }
            retryTimerId_.reset();
        }
    }

    void stop(){
        tryingConnect_ = false;
        state_ = sDisconnected;
        resetTiemrId(true);//删去之前的retry（如果存在的话）
    }

    void setNewConnectionCallback(function<void(int)> cb){
        newConnectionCallback_ = cb;
    }

    void start(){
        assert(!tryingConnect_ && state_==sDisconnected);
        tryingConnect_ = true;
        loop_->runInLoop(bind(&Connector::startInLoop, this));
    }

    //执行restart时有一系列前置条件：start已经被调用过、restart运行在loop线程中、目前已经完成连接的建立并调用了TcpConnection...
    void restart();

private:
    void startInLoop();

    void log_hostAddress(int fd);
    void connectInLoop();

    void handleWrite();

    void handleError();

    void retryInLoop();

    TimeStamp freshRetryTime();

    void resetChannel();

    void tryConnectInLoop();

    EventLoop* loop_;
    function<void(int)> newConnectionCallback_;
    unique_ptr<Channel> channel_;
    InetAddress addr_;
    
    //std::atomic<ConnState> state_;
    ConnState state_;
    bool tryingConnect_;
    int tryNum_;
    int tryIntervalMs_;
    
    unique_ptr<TimerId> retryTimerId_;

    static constexpr int kMaxTryIntervalMs_ = 10000;
};

}
#endif