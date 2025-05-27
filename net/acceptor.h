#ifndef WEBSERVER_NET_ACCEPTOR_H
#define WEBSERVER_NET_ACCEPTOR_H
#include "socket.h"
#include "eventLoop.h"
#include <unistd.h>
namespace webserver{
class Acceptor{
public:
    Acceptor(EventLoop* loop, InetAddress addr, bool reusePort = false):loop_(loop), socket_(Socket::nonblockingSocket()), channel_(new Channel(socket_.fd(), loop)){
        socket_.bind(addr);
        socket_.setReusePort(true);
        if(reusePort){
            socket_.setReusePort(true);
        }
        channel_->setReadableCallback(bind(Acceptor::acceptableCallback, this));
        placeholderFd_ = sockets::createNonblockingSocket();//因为Socket会管理fd的生命周期，因此不能用Socket创建。
    }

    void setNewConnectionCallback(function<void(int)> newConnectionCallback){
        newConnectionCallback_ = newConnectionCallback;
    }

    ~Acceptor(){
        channel_->disableAll();
        channel_->remove();
        ::close(placeholderFd_);
    }
    
    void listen(){
        channel_->assertInLoopThread();
        listening_ = true;
        socket_.listen();
        channel_->enableReading();
    }

    void acceptableCallback(){
        channel_->assertInLoopThread();
        assert(listening_);
        InetAddress addr;
        int connfd = socket_.accept(addr);

        if(connfd != EMFILE){
            assert(newConnectionCallback_);
            newConnectionCallback_(connfd);
        }else{
            LOG_WARN << "The number of file descriptors exceeds the maximum limit." 
                    " Close new connection from "<< addr.toString() <<" automatically.";
            ::close(placeholderFd_);
            int connfd = socket_.accept(addr);
            ::close(connfd);
            placeholderFd_ = sockets::createNonblockingSocket();//貌似createNonblockingSocket里未创建成功会退出
            if(connfd<0 || placeholderFd_<0){
                LOG_FATAL << "Failed in closing new connection and reopening of placeholder file descriptor.";
            }
        }
    }
    
    bool isListening(){
        return listening_;
    }
private:
    Socket socket_;
    unique_ptr<Channel> channel_;
    EventLoop* loop_;
    function<void(int)> newConnectionCallback_;
    bool listening_;
    int placeholderFd_;
};
}
#endif