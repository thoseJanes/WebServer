#include "socket.h"


namespace webserver{

namespace sockets{//不仅为了包装，也为了过滤一些错误。留下待处理的错误。

    int enableFdOption(int fd, int option){
        int oldOption = ::fcntl(fd, F_GETFL);
        int newOption = oldOption | option;
        ::fcntl(fd, F_SETFL, newOption);
        return oldOption;
    }

    int createNonblockingSocket(){
        int fd = ::socket(PF_INET6, SOCK_STREAM, 0);
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
                //各种错误处理，待学习
            }
        }
        return connfd;
    }

    void shutDownWrite(int fd){
        if(::shutdown(fd, SHUT_WR) < 0){
            LOG_ERR << "failed in shutDownWrite().";
        }
    }

    int getSocketError(int fd){
        int err;
        socklen_t len = sizeof(err);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
        return err;
    }



    
}

void Socket::bind(InetAddress addr){sockets::bindOrDie(fd_, addr.getAddr());}
void Socket::listen(){sockets::listenOrDie(fd_, 4096);}
int Socket::accept(InetAddress& addr){return sockets::accept(fd_, addr.getAddr());}
//int connect(InetAddress addr){return detail::connect(fd_, addr.getAddr());}
void Socket::shutDownWrite(){sockets::shutDownWrite(fd_);}

}