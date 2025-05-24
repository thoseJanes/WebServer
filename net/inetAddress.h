#ifndef WEBSERVER_NET_INETADDRESS_H
#define WEBSERVER_NET_INETADDRESS_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string_view>
#include <arpa/inet.h>
#include <stddef.h>
#include <memory.h>
#include "../logging/logger.h"

using namespace std;
namespace webserver{

union UnionAddr{
    sockaddr_in addr;
    sockaddr_in6 addr6;
};

namespace detail{
    sockaddr* toSockaddrPtr(sockaddr_in* addr){
        return static_cast<sockaddr*>(static_cast<void*>(addr));
    }
    sockaddr* toSockaddrPtr(sockaddr_in6* addr){
        return static_cast<sockaddr*>(static_cast<void*>(addr));
    }

    sockaddr_in* toSockaddrInPtr(sockaddr* addr){
        return static_cast<sockaddr_in*>(static_cast<void*>(addr));
    }
    sockaddr_in6* toSockaddrIn6Ptr(sockaddr* addr){
        return static_cast<sockaddr_in6*>(static_cast<void*>(addr));
    }

    void getPeerAddr(int fd, UnionAddr* addr){
        //sockaddr_in6 addr;
        socklen_t addrLen = sizeof(UnionAddr);
        
        if(getpeername(fd, (sockaddr*)addr, &addrLen)<0){
            LOG_FATAL << "getPeerAddr failed";
        }
    }

    void getHostAddr(int fd, UnionAddr* addr){//是否要保证传入值必须得有sockaddr_in6的空间？如何保证？用InetAddress？
        //sockaddr_in6 addr;
        socklen_t addrLen = sizeof(UnionAddr);
        
        if(getsockname(fd, (sockaddr*)addr, &addrLen)<0){
            LOG_FATAL << "getPeerAddr failed";
        }
    }

    bool isSelfConnect(int fd){
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
        }
    }

}

class InetAddress{
public:
    InetAddress(string_view ip, in_port_t port, sa_family_t family){
        static_assert(offsetof(sockaddr_in, sin_family)==offsetof(sockaddr_in6, sin6_family), "sin_family offset values equal");
        static_assert(offsetof(sockaddr_in, sin_port)==offsetof(sockaddr_in6, sin6_port), "sin_port offset values equal");
        //static_assert(offsetof(sockaddr_in, sin_addr)!=offsetof(sockaddr_in6, sin6_addr), "sin_port offset values not equal");
        static_assert(sizeof(addr_) < sizeof(addr6_), "size of addr4_ is smaller than addr6_");
        memset(&addr6_, 0, sizeof(addr6_));
        addr_.sin_family = family;
        addr_.sin_port = htons(port);
        if(family == AF_INET){
            inet_pton(family, ip.data(), &addr_.sin_addr);
        }else{
            inet_pton(family, ip.data(), &addr6_.sin6_addr);
        }
    }

    sa_family_t getFamily(){return addr_.sin_family;}
    UnionAddr* getAddr(){return (UnionAddr*)&addr6_;}
    
    //InetAddress(sockaddr_in addr):addr_(addr){}

private:
    union{
        sockaddr_in addr_;
        sockaddr_in6 addr6_;
    };
};


}


#endif