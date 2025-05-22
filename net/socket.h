#ifndef WEBSERVER_NET_SOCKET_H
#define WEBSERVER_NET_SOCKET_H


#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include "../logging/logger.h"


namespace webserver{
namespace detail{
    void bindOrDie(int fd, sockaddr_in6& addr){
        if(::bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0){
            LOG_FATAL << "bind failed";
        }
    }

    int accept(int fd, sockaddr_in6& addr){
        //sockaddr_storage addr;
        socklen_t addrLen = sizeof(addr);
        int connfd = ::accept(fd, (sockaddr*)&addr, &addrLen);
        if(connfd<0){
            switch(errno){

            }
        }
        return connfd;
    }


}
class Socket{
public:



private:


};

}









#endif