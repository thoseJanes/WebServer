#ifndef WEBSERVER_NET_INETADDRESS_H
#define WEBSERVER_NET_INETADDRESS_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string_view>
#include <arpa/inet.h>
#include <stddef.h>
#include <memory.h>

using namespace std;
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

    sa_family_t addressFamily(){return addr_.sin_family;}
    
    //InetAddress(sockaddr_in addr):addr_(addr){}

private:
    union{
        sockaddr_in addr_;
        sockaddr_in6 addr6_;
    };


};





#endif