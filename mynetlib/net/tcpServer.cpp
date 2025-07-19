#include "tcpServer.h"

using namespace mynetlib;
TcpServer::TcpServer(EventLoop* baseLoop, string_view name, InetAddress addr, bool reusePort)
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

TcpServer::~TcpServer(){
    LOG_DEBUG << "tcpServer "<<name_<<" dtor()";
    for(auto conn:connections_){
        loop_->runInLoop(bind(&TcpConnection::forceClose, conn.second));
    }
}

void TcpServer::newConnection(int sockFd){
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


void TcpServer::removeConnectionInLoop(shared_ptr<TcpConnection> conn){
    loop_->assertInLoopThread();
    auto connIt = connections_.find(conn->nameString());
    connections_.erase(connIt);
}