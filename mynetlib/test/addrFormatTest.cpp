#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory.h>
#include <string>

void ipv4Test(){
    //ipv4 address format
    sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    //addr.sin_addr = in6addr_loopback;
    addr.sin_port = htons(65535);

    char buf[64];
    int addrhl = ntohl(addr.sin_addr.s_addr);
    inet_ntop(addr.sin_family, &addrhl, buf, sizeof buf);
    size_t end = strlen(buf);
    buf[end++] = ':';
    end += snprintf(buf+end, 6, "%d", addr.sin_port);
    buf[end] = '\0';
    printf("%s, size:%ld\n", buf, end);

    auto str = std::string(buf, end);
    printf("%s, size:%ld\n", str.data(), str.size());
}

int main(){
    
    ipv4Test();
    
    //ipv6 address format
    sockaddr_in6 addr6;
    memset(&addr6, 0, sizeof addr6);
    addr6.sin6_family = AF_INET6;
    //addr6.sin6_addr.__in6_u.__u6_addr8 = ;
    addr6.sin6_port = htons(65535);

    char buf[64];
    //int addrhl = ntohl(addr6.sin6_addr.s_addr);
    addr6.sin6_addr = in6addr_loopback;
    inet_ntop(addr6.sin6_family, &addr6.sin6_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    buf[0] = '[';
    buf[end++] = ']'; buf[end++] = ':';
    end += snprintf(buf+end, 6, "%d", ntohs(addr6.sin6_port));
    buf[end] = '\0';
    printf("%s, size:%ld\n", buf, end);

    auto str = std::string(buf, end);
    printf("%s, size:%ld\n", str.data(), str.size());
}