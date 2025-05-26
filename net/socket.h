#ifndef WEBSERVER_NET_SOCKET_H
#define WEBSERVER_NET_SOCKET_H


#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <errno.h>
#include "../logging/logger.h"
#include "inetAddress.h"

namespace webserver{

namespace detail{
int detail::createNonblockingSocket();
}


class Socket{
public:
    explicit Socket(int fd):fd_(fd){}
    int fd() {return fd_;}
    void bind(InetAddress addr);
    void listen();
    int accept(InetAddress& addr);
    //int connect(InetAddress addr){return detail::connect(fd_, addr.getAddr());}
    void shutDownWrite();

    static Socket nonblockingSocket(){return Socket(detail::createNonblockingSocket());}
    void setTcpNoDelay(){
        int optVal = 1;
        if(setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optVal, sizeof(optVal))){
            LOG_FATAL << "failed in Socket::setTcpNoDelay()";
        }
    }
    void setReusePort(){
        int optVal = 1;
        if(setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optVal, sizeof(optVal))){
            LOG_FATAL << "failed in Socket::setReusePort()";
        }
    }
    void setReuseAddr(){
        int optVal = 1;
        if(setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal))){
            LOG_FATAL << "failed in Socket::setReuseAddr()";
        }
    }
    void setKeepAlive(){
        int optVal = 1;
        if(setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optVal, sizeof(optVal))){
            LOG_FATAL << "failed in Socket::setKeepAlive()";
        }
    }

private:
    int fd_;
};

}


#endif