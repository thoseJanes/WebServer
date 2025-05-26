#ifndef WEBSERVER_NET_ACCEPTOR_H
#define WEBSERVER_NET_ACCEPTOR_H
#include "socket.h"
#include "eventLoop.h"
#include <unistd.h>
namespace webserver{
class Acceptor{
public:
    Acceptor(EventLoop* loop, function<void()> readableCallback):loop_(loop), socket_(Socket::nonblockingSocket()), channel_(new Channel(socket_.fd(), loop)){
        channel_->setReadableCallback(readableCallback);
        channel_->enableReading();
    }
    ~Acceptor(){
        channel_->disableAll();
        channel_->remove();
        ::close(socket_.fd());
    }

    void bind(InetAddress addr){
        socket_.bind(addr);
    }

    void listen(){
        channel_->update();
        socket_.listen();
    }

    int accept(InetAddress& addr){
        return socket_.accept(addr);
    }
    
private:
    Socket socket_;
    unique_ptr<Channel> channel_;
    EventLoop* loop_;
};
}
#endif