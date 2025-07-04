#ifndef MYNETLIB_NET_TCPSERVER_H
#define MYNETLIB_NET_TCPSERVER_H

#include "acceptor.h"
#include "tcpConnection.h"
#include "../event/eventThreadPool.h"
#include <functional>
namespace mynetlib{

class TcpServer:Noncopyable{
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

    string getName() const {
        return name_;
    }

    size_t getConnectionNum() const {
        return connections_.size();
    }

private:
    void newConnection(int sockFd){
        loop_->assertInChannelHandling();
        
        char buf[numeric_limits<size_t>::digits10 + 9];
        int len = snprintf(buf, sizeof(buf), ":conn%ld", connId_++);
        auto ioLoop = threadPool_->getNextLoop();
        
        string connName = name_ + buf;
        assert(connections_.find(connName) == connections_.end());

        shared_ptr<TcpConnection> newConn(new TcpConnection(sockFd, connName, ioLoop));
        connections_[connName] = newConn;
        //connections_.insert({connName, newConn});


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
    size_t connId_;

    size_t highWaterBytes_;
    HighWaterCallback highWaterCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    MessageCallback messageCallback_;
    ConnectCallback connectCallback_;
    bool started_;
};

}


#endif