#ifndef WEBSERVER_NET_TCPCLIENT_H
#define WEBSERVER_NET_TCPCLIENT_H

#include "connector.h"
#include "tcpConnection.h"
namespace webserver{

class TcpClient{
public:
    TcpClient(EventLoop* loop, string_view name, InetAddress serverAddress)
    :   loop_(loop), 
        name_(name),
        connector_(new Connector(loop, serverAddress)),
        mutex_()
    {
        connector_->setNewConnectionCallback(bind(&TcpClient::newConnection, this, std::placeholders::_1));
    }

    ~TcpClient(){
        loop_->assertInLoopThread();
        shared_ptr<TcpConnection> conn;
        bool unique;
        {
            MutexLockGuard lock(mutex_);
            unique = connection_.unique();
            conn = connection_;
        }
        if(!conn){
            connector_->stop();
            //loop_->queueInLoop(bind(&Connector::stop, connector_));retry时，connector自己会存活。不需要这里再帮忙?
        }
        if(unique){
            connection_->setCloseCallback(NULL);
            connection_->forceClose();
        }
    }

    void connect(){
        connector_->start();
    }
    
    void setHighWaterCallback(size_t highWaterBytes, HighWaterCallback cb){
        highWaterBytes_ = highWaterBytes;
        highWaterCallback_ = cb;
    }

    void setWriteCompleteCallback(size_t highWaterBytes, WriteCompleteCallback cb){
        writeCompleteCallback_ = cb;
    }

    shared_ptr<TcpConnection> getConnection(){
        MutexLockGuard lock(mutex_);
        return connection_;
    }



private:
    void newConnection(int fd){
        loop_->assertInLoopThread();//如果立即可以连接就不会在channel中进行。而会在pendingFunctor中。
        char buf[name_.size() + numeric_limits<int>::max_digits10 + 12];
        int len = snprintf(buf, sizeof(buf), "%s:conn", name_.c_str());
        {
            MutexLockGuard lock(mutex_);
            connection_.reset(new TcpConnection(fd, string_view(buf, len), loop_));
        }

        connection_->setMessageCallback(detail::defaultMessageCallback);
        connection_->setConnectCallback(detail::defaultConnectCallback);
        if(highWaterCallback_){
            connection_->setHighWaterCallback(highWaterBytes_, highWaterCallback_);
        }
        if(writeCompleteCallback_){
            connection_->setWriteCompleteCallback(writeCompleteCallback_);
        }

        //注意，在析构时会先析构自己，再析构成员。所以如果close在TcpClient析构之后会出错。需要先重置connectionClosed函数。
        connection_->setCloseCallback(bind(&TcpClient::connectionClosed, this));
    }

    void connectionClosed(){
        assert(connection_);
        {
            MutexLockGuard lock(mutex_);
            connection_.reset();
        }
        loop_->runInLoop(bind(&Connector::restart, connector_));
    }
    
    EventLoop* loop_;
    shared_ptr<Connector> connector_;
    shared_ptr<TcpConnection> connection_;
    MutexLock mutex_;

    size_t highWaterBytes_;
    HighWaterCallback highWaterCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    string name_;
};

}
#endif