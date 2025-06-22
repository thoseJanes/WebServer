#ifndef WEBSERVER_NET_TCPCLIENT_H
#define WEBSERVER_NET_TCPCLIENT_H

#include "connector.h"
#include "tcpConnection.h"
namespace webserver{

class TcpClient:Noncopyable{
public:
    TcpClient(EventLoop* loop, string_view name, InetAddress serverAddress, bool enableRetry = true)
    :   loop_(loop), 
        name_(name),
        connector_(new Connector(loop, serverAddress)),
        mutex_(),
        connectCallback_(detail::defaultConnectCallback),
        messageCallback_(detail::defaultMessageCallback),
        retry_(enableRetry),
        connId_(0)
    {
        connector_->setNewConnectionCallback(bind(&TcpClient::newConnection, this, std::placeholders::_1));
    }
 
    ~TcpClient(){
        //loop_->assertInLoopThread();
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
        connect_ = true;
        connector_->start();
    }

    //返回是否成功，只有在存在连接时，才能成功
    bool disConnect(){
        connect_ = false;
        MutexLockGuard lock(mutex_);
        if(connection_){
            connection_->shutdownWrite();
            return true;
        }else{
            return false;
        }
    }
    //尽力。应当用在retry期间。
    void stopConnector(){
        connect_ = false;
        connector_->stop();
    }
    
    void setHighWaterCallback(size_t highWaterBytes, HighWaterCallback cb){
        highWaterBytes_ = highWaterBytes;
        highWaterCallback_ = cb;
    }

    void setWriteCompleteCallback(WriteCompleteCallback cb){
        writeCompleteCallback_ = cb;
    }

    void setMessageCallback(MessageCallback cb){
        messageCallback_ = cb;
    }

    void setConnectCallback(ConnectCallback cb){
        connectCallback_ = cb;
    }

    shared_ptr<TcpConnection> getConnection(){
        MutexLockGuard lock(mutex_);
        return connection_;
    }



private:
    void newConnection(int fd){
        loop_->assertInLoopThread();//如果立即可以连接就不会在channel中进行。而会在pendingFunctor中。
        char buf[name_.size() + numeric_limits<int>::max_digits10 + 12];
        int len = snprintf(buf, sizeof(buf), "%s:conn%d", name_.c_str(), connId_++);
        LOG_DEBUG << "client create connection " << buf;
        {
            MutexLockGuard lock(mutex_);
            connection_.reset(new TcpConnection(fd, string_view(buf, len), loop_));
        }

        connection_->setMessageCallback(messageCallback_);
        connection_->setConnectCallback(connectCallback_);
        if(highWaterCallback_){
            connection_->setHighWaterCallback(highWaterBytes_, highWaterCallback_);
        }
        if(writeCompleteCallback_){
            connection_->setWriteCompleteCallback(writeCompleteCallback_);
        }

        //注意，在析构时会先析构自己，再析构成员。所以如果close在TcpClient析构之后会出错。需要先重置connectionClosed函数。
        LOG_DEBUG << "client new connection";
        connection_->setCloseCallback(bind(&TcpClient::connectionClosed, this));
        connection_->connectEstablished();
    }

    void connectionClosed(){
        loop_->assertInLoopThread();
        assert(connection_);
        {
            MutexLockGuard lock(mutex_);
            connection_.reset();
        }
        if(retry_ && connect_){
            connector_->restart();
        }
        
    }
    
    EventLoop* loop_;
    shared_ptr<Connector> connector_;
    shared_ptr<TcpConnection> connection_;
    MutexLock mutex_;

    size_t highWaterBytes_;
    HighWaterCallback highWaterCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    MessageCallback messageCallback_;
    ConnectCallback connectCallback_;
    string name_;

    int connId_;

    //状态
    bool connect_;
    bool retry_;
};

}
#endif