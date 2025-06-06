#ifndef WEBSERVER_NET_TCPSERVER_H
#define WEBSERVER_NET_TCPSERVER_H

#include "acceptor.h"
#include "tcpConnection.h"
#include "../event/eventThreadPool.h"
#include <functional>
namespace webserver{

class TcpServer{
public:
    typedef EventThread::ThreadInitCallback ThreadInitCallback;
    TcpServer(EventLoop* baseLoop, string_view name, InetAddress addr, bool reusePort)
    :   loop_(baseLoop), 
        name_(name),
        acceptor_(new Acceptor(loop_, addr, reusePort)),
        threadPool_(new EventThreadPool(baseLoop, name)),
        connId_(0),
        messageCallback_(detail::defaultMessageCallback),
        connectCallback_(detail::defaultConnectCallback),
        started_(false)
    {
        acceptor_->setNewConnectionCallback(bind(&TcpServer::newConnection, this, placeholders::_1));
    }
    ~TcpServer(){
        LOG_DEBUG << "tcpServer "<<name_<<" dtor()";
        for(auto conn:connections_){
            loop_->runInLoop(bind(&TcpConnection::forceClose, conn.second));
        }
    }
    
    void setHighWaterCallback(size_t highWaterBytes, HighWaterCallback cb){
        assert(!started_);
        highWaterCallback_ = cb;
        highWaterBytes_ = highWaterBytes;
    }
    void setWriteCompleteCallback(WriteCompleteCallback cb){
        assert(!started_);
        writeCompleteCallback_ = cb;
    }

    void setMessageCallback(MessageCallback cb){
        assert(!started_);
        messageCallback_ = cb;
    }

    void setConnectCallback(ConnectCallback cb){
        assert(!started_);
        connectCallback_ = cb;
    }
    
    void start(int threadNum, EventThread::ThreadInitCallback initThreadCallback){
        started_ = true;
        threadPool_->start(threadNum, initThreadCallback);//如果创建较多线程，貌似会在这里阻塞一会儿。是否需要放入loop运行？
        loop_->runInLoop(bind(&TcpServer::startInLoop, this));
    }
    void startInLoop(){
        loop_->assertInLoopThread();
        acceptor_->listen();
        LOG_INFO << "Server " << name_ << " start listen";
    }

    bool isStarted(){
        return started_;
    }

private:
    void newConnection(int sockFd){
        loop_->assertInChannelHandling();
        
        char buf[name_.size() + numeric_limits<int>::max_digits10 + 12];
        int len = snprintf(buf, sizeof(buf), "%s:conn%d", name_.c_str(), connId_++);
        auto ioLoop = threadPool_->getNextLoop();
        shared_ptr<TcpConnection> newConn(new TcpConnection(sockFd, string_view(buf, len), ioLoop));
        assert(connections_.find(string(buf)) == connections_.end());
        connections_.insert({string(buf), newConn});


        newConn->setMessageCallback(messageCallback_);
        newConn->setConnectCallback(connectCallback_);
        newConn->setCloseCallback(bind(&TcpServer::removeConnection, this, placeholders::_1));
        if(highWaterCallback_){
            newConn->setHighWaterCallback(highWaterBytes_, highWaterCallback_);
        }
        if(writeCompleteCallback_){
            newConn->setWriteCompleteCallback(writeCompleteCallback_);
        }
        LOG_DEBUG << "connection from " << newConn->peerAddressString() << " init. name " << string(buf);
        ioLoop->runInLoop(bind(&TcpConnection::connectEstablished, newConn));
    }

    void removeConnection(shared_ptr<TcpConnection> conn){
        loop_->runInLoop(bind(&TcpServer::removeConnectionInLoop, this, conn));//注意，这是由于removeConnection线程处于ioLoop中，但需要在baseLoop中更新连接列表
    }

    void removeConnectionInLoop(shared_ptr<TcpConnection> conn){
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
    MessageCallback messageCallback_;
    ConnectCallback connectCallback_;
    bool started_;
};

}


#endif