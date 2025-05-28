#ifndef WEBSERVER_NET_TCPCONNECTION_H
#define WEBSERVER_NET_TCPCONNECTION_H

#include "channel.h"
#include "inetAddress.h"
#include "connBuffer.h"
#include "eventLoop.h"
#include <functional>
#include <memory>
namespace webserver{

class TcpConnection{
public:
    TcpConnection(int fd, EventLoop* loop)
    :   loop_(loop), 
        channel_(new Channel(fd, loop)),
        hostAddr_(),
        peerAddr_()
    {
        sockets::getHostAddr(fd, hostAddr_.getAddr());
        sockets::getPeerAddr(fd, hostAddr_.getAddr());
        channel_->setCloseCallback(closeCallback_);
        channel_->setErrorCallback(errorCallback_);
        channel_->setReadableCallback(bind(&TcpConnection::handleRead, this));
        channel_->setWritableCallback(bind(&TcpConnection::handleWrite, this));
        channel_->enableReading();
        channel_->enableWritting();
    }

    void send(string_view str){
        if(loop_->isInLoopThread()){
            sendInLoop(str.data(), str.size());
        }else{
            loop_->runInLoop(bind<void(TcpConnection::*)(string)>(&TcpConnection::sendInLoop, this, string(str)));
        }
    }

    void send(ConnBuffer* buffer){
        if(loop_->isInLoopThread()){
            sendInLoop(buffer->retrieveAllAsString());
        }else{
            loop_->runInLoop(bind<void(TcpConnection::*)(string)>(&TcpConnection::sendInLoop, this, buffer->retrieveAllAsString()));
        }
    }

    void sendInLoop(string str){
        sendInLoop(str.data(), str.size());
    }

    void sendInLoop(const char* data, size_t len){
        loop_->assertInLoopThread();
        //什么情况下可以直接发送数据?
        outputBuffer_.append(data, len);
    }


    void handleWrite(){
        loop_->assertInChannelHandling();
        auto remain = outputBuffer_.readableBytes();
        if(remain > 0){
            ssize_t n = ::send(channel_->getFd(), outputBuffer_.readerBegin(), outputBuffer_.readableBytes(), NULL);
            remain -= static_cast<size_t>(n);
            if(remain==0 && writeCompleteCallback_){
                writeCompleteCallback_();
            }
        }
    }
    void handleRead(){
        loop_->assertInChannelHandling();
        inputBuffer_.readFromFd(channel_->getFd());
        if(messageCallback_){
            messageCallback_(inputBuffer_.retrieveAllAsString());
        }
        //inputBuffer_.retrieveAll();???是否需要？
    }


    ~TcpConnection();
private:
    EventLoop* loop_;
    unique_ptr<Channel> channel_;
    InetAddress hostAddr_;
    InetAddress peerAddr_;
    ConnBuffer inputBuffer_;
    ConnBuffer outputBuffer_;

    std::function<void(string data)> messageCallback_;//读到数据
    std::function<void()> writeCompleteCallback_;//写完数据
    std::function<void()> closeCallback_;
    std::function<void()> errorCallback_;
};

}


#endif