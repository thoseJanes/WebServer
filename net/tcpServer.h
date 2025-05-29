#ifndef WEBSERVER_NET_TCPSERVER_H
#define WEBSERVER_NET_TCPSERVER_H

#include "acceptor.h"
#include "tcpConnection.h"
#include "../event/eventThreadPool.h"
#include <functional>
namespace webserver{

class TcpServer{
public:
    TcpServer(EventLoop* baseLoop, string_view name, InetAddress addr, bool reusePort)
    :   loop_(baseLoop), 
        name_(name),
        acceptor_(new Acceptor(loop_, addr, reusePort)),
        threadPool_(new EventThreadPool(baseLoop, name)),
        connId_(0)
    {
        acceptor_->setNewConnectionCallback(bind(&TcpServer::newConnection, this, placeholders::_1));
    }
    ~TcpServer(){
        for(auto conn:connections_){
            loop_->runInLoop(bind(&TcpConnection::forceClose, conn.second));
        }
    }
    
    void setHighWaterCallback(size_t highWaterBytes, HighWaterCallback cb){
        highWaterCallback_ = cb;
        highWaterBytes_ = highWaterBytes;
    }
    void setWriteCompleteCallback(size_t highWaterBytes, WriteCompleteCallback cb){
        writeCompleteCallback_ = cb;
    }
    
    void start(int threadNum, function<void()> initThreadCallback){
        threadPool_->start(threadNum, initThreadCallback);//如果创建较多线程，貌似会在这里阻塞一会儿。是否需要放入loop运行？
        loop_->runInLoop(bind(&TcpServer::startInLoop, this));
    }
    void startInLoop(){
        loop_->assertInLoopThread();
        acceptor_->listen();
    }

private:
    void newConnection(int sockFd){
        loop_->assertInChannelHandling();
        
        char buf[name_.size() + numeric_limits<int>::max_digits10 + 12];
        int len = snprintf(buf, sizeof(buf), "%s:conn%d", name_.c_str(), connId_);
        shared_ptr<TcpConnection> newConn(new TcpConnection(sockFd, string_view(buf, len-1), threadPool_->getNextLoop()));
        connections_.insert({string(buf, len-1), newConn});
        connId_++;

        newConn->setMessageCallback(detail::defaultMessageCallback);
        newConn->setConnectCallback(detail::defaultConnectCallback);
        newConn->setCloseCallback(bind(&TcpServer::removeConnection, this, placeholders::_1));
        if(highWaterCallback_){
            newConn->setHighWaterCallback(highWaterBytes_, highWaterCallback_);
        }
        if(writeCompleteCallback_){
            newConn->setWriteCompleteCallback(writeCompleteCallback_);
        }
    }

    void removeConnection(shared_ptr<TcpConnection> conn){
        loop_->assertInLoopThread();
        auto connIt = connections_.find(conn->nameString());
        connections_.erase(connIt);
    }

    EventLoop* loop_;
    unique_ptr<EventThreadPool> threadPool_;
    unique_ptr<Acceptor> acceptor_;
    map<string, shared_ptr<TcpConnection>> connections_;
    string name_;
    int connId_;

    size_t highWaterBytes_;
    HighWaterCallback highWaterCallback_;
    WriteCompleteCallback writeCompleteCallback_;
};

}


#endif