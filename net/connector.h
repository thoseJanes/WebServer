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
    void restart(){
        loop_->assertInLoopThread();
        assert(tryingConnect_ == false);
        assert(state_ == sConnected);
        // if(state_ != sConnected){
        //     LOG_DEBUG<< "connector state:" << static_cast<int>(state_);
        // }
        LOG_DEBUG << "restart connector";
        tryingConnect_ = true;
        tryIntervalMs_ = 500;
        //当前fd已经交给TcpConnection管理了，TcpConnection析构时会关闭它.
        connectInLoop();//loop_->runInLoop(bind(&Connector::connectInLoop, this));
    }

private:
    void startInLoop(){
        tryIntervalMs_ = 500;
        connectInLoop();
    }

    void log_hostAddress(int fd){
        InetAddress testAddr;
        sockets::getHostAddr(fd, testAddr.getAddr());
        LOG_DEBUG << "Try connect in addr " << testAddr.toString();
    }

    void connectInLoop(){
        loop_->assertInLoopThread();
        int fd = sockets::createNonblockingSocket(addr_.getFamily());
        int ret = ::connect(fd, (sockaddr*)addr_.getAddr(), sizeof(addr_));
        log_hostAddress(fd);
        LOG_TRACE << "Connector starts connect";
        int err = ret==0?0:errno;
        switch(err){
            case 0:
            case EINPROGRESS://正在等待建立连接
                //LOG_DEBUG << "waitting for connecting";
                state_ = sConnecting;
                channel_.reset(new Channel(fd, loop_));
                channel_->setWritableCallback(bind(&Connector::handleWrite, this));
                channel_->setErrorCallback(bind(&Connector::handleError, this));
                channel_->enableWriting();//如果不放在loop中，可能丢失事件。
                break;
            default:
                close(channel_->getFd());
                LOG_SYSERROR << "Unknow error";
                break;
        }
    }

    void handleWrite(){
        loop_->assertInChannelHandling();
        assert(channel_);
        channel_->disableAll();
        channel_->remove();//在connectInLoop中reset
        
        if(sockets::getSocketError(channel_->getFd())!=0 || sockets::isSelfConnect(channel_->getFd())){
            retryInLoop();//继续尝试连接。
        }else{
            //成功建立连接，可能需要resetChannel，fd交给TcpConnection管理。（其实不resetChannel也没关系，可以等待析构时关闭或者restart时自动关闭）
            state_ = sConnected;
            tryingConnect_ = false;
            tryIntervalMs_ = 500;//一旦重新连接成功就重置。
            newConnectionCallback_(channel_->getFd());
        }
    }

    void handleError(){
        loop_->assertInChannelHandling();
        channel_->disableAll();
        channel_->remove();
        if(tryingConnect_){
            retryInLoop();
        }
    }

    void retryInLoop(){
        ::close(channel_->getFd());//立马关闭，防止后面又成功连接？
        state_ = sDisconnected;
        if(tryingConnect_){
            loop_->queueInLoop(bind(&Connector::resetChannel, this));//这个在接下来的pendingFunctor就会触发。

            TimeStamp tryStamp = freshRetryTime();
            auto timerId = loop_->runAt(tryStamp, bind(&Connector::tryConnectInLoop, shared_from_this()));//这个在下一次channel才会触发。
            assert(!retryTimerId_);
            retryTimerId_.reset(new TimerId(timerId));
        }
    }

    TimeStamp freshRetryTime(){
        TimeStamp tryStamp = TimeStamp::now();
        //随机初始间隔。
        if(tryIntervalMs_ <= 500){
            std::srand(static_cast<uint>(tryStamp.getMicroSecondsSinceEpoch()));
            tryIntervalMs_ = 500 + std::rand()%500;
        }
        tryStamp.add(tryIntervalMs_*1000);
        LOG_DEBUG << "Connecting failed. Retry in " << tryIntervalMs_/1000.0;
        tryIntervalMs_ = min(2*tryIntervalMs_, kMaxTryIntervalMs_);
        return tryStamp;
    }

    void resetChannel(){
        loop_->assertInPendingFunctors();
        channel_.reset();
    }

    void tryConnectInLoop(){
        LOG_DEBUG << "Start retry";
        loop_->assertInChannelHandling();
        resetTiemrId(false);//当前timer已经不存在了。
        if(!tryingConnect_ || state_!=sDisconnected){
            return;
        }
        connectInLoop();//loop_->runInLoop(bind(&Connector::connectInLoop, this));
    }

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