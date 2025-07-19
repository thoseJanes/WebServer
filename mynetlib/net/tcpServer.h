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
    TcpServer(EventLoop* baseLoop, string_view name, InetAddress addr, bool reusePort);
    ~TcpServer();
    
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
    void newConnection(int sockFd);

    void removeConnection(shared_ptr<TcpConnection> conn){
        loop_->runInLoop(bind(&TcpServer::removeConnectionInLoop, this, conn));//注意，这是由于removeConnection线程处于ioLoop中，但需要在baseLoop中更新连接列表
    }

    void removeConnectionInLoop(shared_ptr<TcpConnection> conn);

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