#ifndef MYNETLIB_NET_SOCKET_H
#define MYNETLIB_NET_SOCKET_H


#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <errno.h>
#include "../logging/logger.h"
#include "inetAddress.h"
#include <unistd.h>

namespace mynetlib{

namespace sockets{
int createNonblockingSocket(sa_family_t family);
int connect(int fd, UnionAddr* addr);
int getSocketError(int fd);
}


class Socket:Noncopyable{
public:
    explicit Socket(int fd):fd_(fd){}
    ~Socket(){::close(fd_);}
    int fd() {return fd_;}
    void bind(InetAddress addr);
    void listen();
    int accept(InetAddress& addr);
    //int connect(InetAddress addr){return detail::connect(fd_, addr.getAddr());}
    void shutDownWrite();

    static Socket nonblockingSocket(sa_family_t family){return Socket(sockets::createNonblockingSocket(family));}
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

    // ssize_t send(const char* data, size_t len){
    //     return ::send(fd_, data, len, NULL);
    // }
    ssize_t write(const char* data, size_t len){
        return ::write(fd_, data, len);
    }

    InetAddress getPeerAddr() const {
        InetAddress addr;
        memset(&addr, 0, sizeof(addr));
        sockets::getPeerAddr(fd_, addr.getAddr());
        return addr;
    }

    InetAddress getHostAddr() const {
        InetAddress addr;
        memset(&addr, 0, sizeof(addr));
        sockets::getHostAddr(fd_, addr.getAddr());
        return addr;
    }
private:
    int fd_;
};

}


#endif