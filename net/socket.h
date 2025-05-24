#ifndef WEBSERVER_NET_SOCKET_H
#define WEBSERVER_NET_SOCKET_H


#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include "../logging/logger.h"
#include "inetAddress.h"


namespace webserver{
namespace detail{//不仅为了包装，也为了过滤一些错误。留下待处理的错误。

    union UnionAddr{
        sockaddr_in addr;
        sockaddr_in6 addr6;
    };

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

    void getPeerAddr(int fd, UnionAddr* addr){
        //sockaddr_in6 addr;
        socklen_t addrLen = sizeof(UnionAddr);
        memset(addr, 0, addrLen);
        
        if(getpeername(fd, (sockaddr*)addr, &addrLen)<0){
            LOG_FATAL << "getPeerAddr failed";
        }
    }

    void getHostAddr(int fd, UnionAddr* addr){//是否要保证传入值必须得有sockaddr_in6的空间？如何保证？用InetAddress？
        //sockaddr_in6 addr;
        socklen_t addrLen = sizeof(UnionAddr);
        memset(addr, 0, addrLen);
        
        if(getsockname(fd, (sockaddr*)addr, &addrLen)<0){
            LOG_FATAL << "getPeerAddr failed";
        }
    }

    bool isLoopbackSocket(int fd){
        UnionAddr hAddr;
        UnionAddr pAddr;
        getHostAddr(fd, &hAddr);
        getPeerAddr(fd, &pAddr);
        if(hAddr.addr.sin_family == AF_INET){
            return hAddr.addr.sin_addr.s_addr == pAddr.addr.sin_addr.s_addr 
                    && hAddr.addr.sin_port == pAddr.addr.sin_port;
        }else if(hAddr.addr6.sin6_family == AF_INET6){
            return memcmp(&hAddr.addr6.sin6_addr, &pAddr.addr6.sin6_addr, sizeof(in6_addr))
                    && hAddr.addr6.sin6_port == pAddr.addr6.sin6_port;
        }else{
            LOG_FATAL << "unknow address faimily.";
        }
    }

}
class Socket{
public:
    explicit Socket(int fd):fd_(fd){}
    int fd() {return fd_;}
    void bind(InetAddress addr){return detail::bindOrDie(fd_, (detail::UnionAddr*)&addr);}
    int connect(InetAddress addr){return detail::connect(fd_, (detail::UnionAddr*)&addr);}


private:
    int fd_;

};

}









#endif