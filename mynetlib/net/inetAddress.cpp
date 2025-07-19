#include "inetAddress.h"

using namespace mynetlib;

sockaddr* sockets::toSockaddrPtr(sockaddr_in* addr){
    return static_cast<sockaddr*>(static_cast<void*>(addr));
}
sockaddr* sockets::toSockaddrPtr(sockaddr_in6* addr){
    return static_cast<sockaddr*>(static_cast<void*>(addr));
}

sockaddr_in* sockets::toSockaddrInPtr(sockaddr* addr){
    return static_cast<sockaddr_in*>(static_cast<void*>(addr));
}
sockaddr_in6* sockets::toSockaddrIn6Ptr(sockaddr* addr){
    return static_cast<sockaddr_in6*>(static_cast<void*>(addr));
}

void sockets::getPeerAddr(int fd, UnionAddr* addr){
    //sockaddr_in6 addr;
    socklen_t addrLen = sizeof(UnionAddr);
    
    if(getpeername(fd, (sockaddr*)addr, &addrLen)<0){
        LOG_SYSERROR << "getPeerAddr failed";
    }
}

void sockets::getHostAddr(int fd, UnionAddr* addr){//是否要保证传入值必须得有sockaddr_in6的空间？如何保证？用InetAddress？
    //sockaddr_in6 addr;
    socklen_t addrLen = sizeof(UnionAddr);
    
    if(getsockname(fd, (sockaddr*)addr, &addrLen)<0){
        LOG_SYSERROR << "getPeerAddr failed";
    }
}

bool sockets::isSelfConnect(int fd){
    UnionAddr hAddr; memset(&hAddr, 0, sizeof(UnionAddr));
    UnionAddr pAddr; memset(&pAddr, 0, sizeof(UnionAddr));
    getHostAddr(fd, &hAddr);
    getPeerAddr(fd, &pAddr);
    if(hAddr.addr.sin_family == AF_INET){
        return hAddr.addr.sin_addr.s_addr == pAddr.addr.sin_addr.s_addr 
                && hAddr.addr.sin_port == pAddr.addr.sin_port;
    }else if(hAddr.addr6.sin6_family == AF_INET6){
        return memcmp(&hAddr.addr6.sin6_addr, &pAddr.addr6.sin6_addr, sizeof(in6_addr))
                && hAddr.addr6.sin6_port == pAddr.addr6.sin6_port;
    }else{
        LOG_FATAL << "unknow address faimily:" << hAddr.addr.sin_family << ".";
        abort();
    }
}

