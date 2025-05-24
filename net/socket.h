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
namespace detail{//不仅为了包装，也为了过滤一些错误。留下待处理的错误。

    int enableFdOption(int fd, int option){
        int oldOption = fcntl(fd, F_GETFL);
        int newOption = oldOption | option;
        fcntl(fd, F_SETFL, newOption);
        return oldOption;
    }

    int createNonblockingSocket(){
        int fd = socket(PF_INET6, SOCK_STREAM, 0);
        if(fd<0){
            LOG_FATAL << "create socket failed";
        }
        enableFdOption(fd, O_NONBLOCK | O_CLOEXEC);
        return fd;
    }

    void bindOrDie(int fd, UnionAddr* addr){
        if(::bind(fd, (sockaddr*)addr, sizeof(UnionAddr)) < 0){
            LOG_FATAL << "bind failed";
        }
    }

    void listenOrDie(int fd, int max){
        if(::listen(fd, max)<0){
            LOG_FATAL << "listen failed";
        }
    }

    int accept(int fd, UnionAddr* addr){
        //sockaddr_storage addr;
        socklen_t addrLen = sizeof(UnionAddr);
        int connfd = ::accept(fd, (sockaddr*)addr, &addrLen);
        if(connfd<0){
            switch(errno){

            }
        }
        return connfd;
    }

    int connect(int fd, UnionAddr* addr){
        return ::connect(fd, (sockaddr*)addr, sizeof(UnionAddr))<0;
    }

    void shutDownWrite(int fd){
        if(::shutdown(fd, SHUT_WR) < 0){
            LOG_ERR << "failed in shutDownWrite().";
        }
    }

    //void read
}

class Socket{
public:
    explicit Socket(int fd):fd_(fd){}
    int fd() {return fd_;}
    void bind(InetAddress addr){detail::bindOrDie(fd_, addr.getAddr());}
    void listen(){detail::listenOrDie(fd_, 4096);}
    int accept(InetAddress& addr){detail::accept(fd_, addr.getAddr());}
    //int connect(InetAddress addr){return detail::connect(fd_, addr.getAddr());}
    void shutDownWrite(){detail::shutDownWrite(fd_);}




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