#ifndef WEBSERVER_NET_CONNECTOR_H
#define WEBSERVER_NET_CONNECTOR_H

#include "../event/channel.h"
#include "../event/eventLoop.h"
#include "socket.h"

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
        if(channel_){
            if(channel_->isWritingEnabled()){
                channel_->disableAll();
                channel_->remove();
            }
            channel_.reset();//这貌似也不安全？如果刚好在执行channel呢？为啥这些东西好像都不安全？connector和它的channel也不是在同一个loop里吧？
            //需要放进pendingFunctors中析构吗？当然需要先删除定时器？
        }
        //貌似还是需要删除定时器，怕析构之后执行回调发生未定义行为。
    }

    void stop(){
        tryingConnect_ = false;
        state_ = sDisconnected;
    }

    void setNewConnectionCallback(function<void(int)> cb){
        newConnectionCallback_ = cb;
    }

    void start(){
        assert(!tryingConnect_ && state_==sDisconnected);
        tryingConnect_ = true;
        loop_->runInLoop(bind(&Connector::connectInLoop, this));
    }

    //执行restart时有一系列前置条件：start已经被调用过、restart运行在loop线程中、目前已经完成连接的建立并调用了TcpConnection...
    void restart(){
        loop_->assertInLoopThread();
        assert(state_ = sConnected);
        tryingConnect_ = true;
        tryIntervalMs_ = 500;
        //当前fd已经交给TcpConnection管理了，TcpConnection析构时会关闭它.
        connectInLoop();//loop_->runInLoop(bind(&Connector::connectInLoop, this));
        
    }

private:
    void connectInLoop(){
        loop_->assertInLoopThread();
        int fd = sockets::createNonblockingSocket(addr_.getFamily());
        int ret = ::connect(fd, (sockaddr*)addr_.getAddr(), sizeof(addr_));

        if(ret == 0){//立即可连接。
            state_ = sConnected;
            tryingConnect_ = false;
            newConnectionCallback_(fd);
            tryIntervalMs_ = 500;
        }else{//连接无法立即建立。
            int err = errno;
            switch(err){
                case EINPROGRESS://正在等待建立连接
                    state_ = sConnecting;
                    channel_.reset(new Channel(fd, loop_));
                    channel_->setWritableCallback(bind(&Connector::handleWrite, this));
                    channel_->setErrorCallback(bind(&Connector::handleError, this));
                    channel_->enableWriting();//如果不放在loop中，可能丢失事件。
                    break;
            }
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
            newConnectionCallback_(channel_->getFd());
            tryIntervalMs_ = 500;
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
        state_ = sDisconnected;
        TimeStamp tryStamp = TimeStamp::now();
        tryStamp.add(tryIntervalMs_*1000);
        retryTimerId_ = loop_->runAt(tryStamp, bind(&Connector::tryConnectInLoop, shared_from_this()));//这个在下一次channel才会触发。
        loop_->queueInLoop(bind(&Connector::resetChannelAndCloseFd, this));//这个在接下来的pendingFunctor就会触发。
        tryIntervalMs_ *= 2;
    }

    void resetChannelAndCloseFd(){
        loop_->assertInPendingFunctors();
        ::close(channel_->getFd());
        channel_.reset();
    }

    void tryConnectInLoop(){
        loop_->assertInChannelHandling();
        if(!tryingConnect_ || state_!=sDisconnected){
            return;
        }
        connectInLoop();//loop_->runInLoop(bind(&Connector::connectInLoop, this));
    }

    EventLoop* loop_;
    function<void(int)> newConnectionCallback_;
    unique_ptr<Channel> channel_;
    InetAddress addr_;
    
    ConnState state_;
    bool tryingConnect_;
    int tryNum_;
    int tryIntervalMs_;
    TimerId retryTimerId_;


    

};

}
#endif