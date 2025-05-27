#ifndef WEBSERVER_NET_SOCKET_H
#define WEBSERVER_NET_SOCKET_H


#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <errno.h>
#include "../logging/logger.h"
#include "inetAddress.h"
#include <unistd.h>

namespace webserver{

namespace sockets{
int createNonblockingSocket();
int connect(int fd, UnionAddr* addr);
}


class Socket{
public:
    explicit Socket(int fd):fd_(fd){}
    ~Socket(){::close(fd_);}
    int fd() {return fd_;}
    void bind(InetAddress addr);
    void listen();
    int accept(InetAddress& addr);
    //int connect(InetAddress addr){return detail::connect(fd_, addr.getAddr());}
    void shutDownWrite();

    static Socket nonblockingSocket(){return Socket(detail::createNonblockingSocket());}
    void setTcpNoDelay(bool val){
        int optVal = val;
        if(setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optVal, sizeof(optVal))){
            LOG_FATAL << "failed in Socket::setTcpNoDelay()";
        }
    }
    void setReusePort(bool val){
        int optVal = val;
        if(setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optVal, sizeof(optVal))){
            LOG_FATAL << "failed in Socket::setReusePort()";
        }
    }
    void setReuseAddr(bool val){
        int optVal = val;
        if(setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal))){
            LOG_FATAL << "failed in Socket::setReuseAddr()";
        }
    }
    void setKeepAlive(bool val){
        int optVal = val;
        if(setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optVal, sizeof(optVal))){
            LOG_FATAL << "failed in Socket::setKeepAlive()";
        }
    }

private:
    int fd_;
};

}


#endif