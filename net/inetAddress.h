#ifndef WEBSERVER_NET_INETADDRESS_H
#define WEBSERVER_NET_INETADDRESS_H

// #include <sys/socket.h>

// #include <stdint.h>

// #include <stddef.h>
// #include <memory.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string_view>
#include "../logging/logger.h"

//INADDR_LOOPBACK
//IN6ADDR_LOOPBACK_INIT
using namespace std;
namespace webserver{

union UnionAddr{
    sockaddr_in addr;
    sockaddr_in6 addr6;
};

namespace sockets{
    sockaddr* toSockaddrPtr(sockaddr_in* addr);
    sockaddr* toSockaddrPtr(sockaddr_in6* addr);

    sockaddr_in* toSockaddrInPtr(sockaddr* addr);
    sockaddr_in6* toSockaddrIn6Ptr(sockaddr* addr);

    void getPeerAddr(int fd, UnionAddr* addr);
    //是否要保证传入值必须得有sockaddr_in6的空间？如何保证？用InetAddress？
    void getHostAddr(int fd, UnionAddr* addr);

    bool isSelfConnect(int fd);
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

    InetAddress(in_addr_t ip = INADDR_ANY, in_port_t port = 0){
        memset(&addr6_, 0, sizeof(addr6_));
        addr_.sin_addr.s_addr = ip;
        addr_.sin_port = port;
        addr_.sin_family = AF_INET;
    }

    string toString() const {
        char buf[64];
        if(addr_.sin_family == AF_INET){
            inet_ntop(addr_.sin_family, &addr_.sin_addr, buf, sizeof(buf));
            size_t end = strlen(buf);
            buf[end++] = ':';
            end += detail::formatInteger(buf+end, ntohs(addr_.sin_port));
            buf[end] = '\0';
            return string(buf, end);
        }else if(addr_.sin_family == AF_INET6){
            buf[0] = '[';
            inet_ntop(addr6_.sin6_family, &addr6_.sin6_addr, buf, sizeof(buf));
            size_t end = strlen(buf);
            buf[end++] = ']'; 
            buf[end++] = ':';
            end += detail::formatInteger(buf+end, ntohs(addr_.sin_port));
            buf[end] = '\0';
            return string(buf, end);
        }else{
            return "[unknowAddress]";
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