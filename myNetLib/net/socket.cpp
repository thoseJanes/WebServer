#include "socket.h"
#include "../process/currentThread.h"

namespace mynetlib{

namespace sockets{//不仅为了包装，也为了过滤一些错误。留下待处理的错误。

    int enableFdOption(int fd, int option){
        int oldOption = ::fcntl(fd, F_GETFL);
        int newOption = oldOption | option;
        ::fcntl(fd, F_SETFL, newOption);
        return oldOption;
    }

    int createNonblockingSocket(sa_family_t family){
        int fd = ::socket(family, SOCK_STREAM, 0);
        if(fd<0){
            LOG_SYSFATAL << "create socket failed";
        }
        enableFdOption(fd, O_NONBLOCK | O_CLOEXEC);
        return fd;
    }

    void bindOrDie(int fd, UnionAddr* addr){
        if(::bind(fd, (sockaddr*)addr, sizeof(UnionAddr)) < 0){
            LOG_SYSFATAL << "bind failed";
        }
    }

    void listenOrDie(int fd, int max){
        if(::listen(fd, max)<0){
            LOG_SYSFATAL << "listen failed";
        }
    }

    int accept(int fd, UnionAddr* addr){
        //sockaddr_storage addr;
        socklen_t addrLen = sizeof(UnionAddr);
        int connfd = ::accept(fd, (sockaddr*)addr, &addrLen);
        if(connfd<0){
            int err = errno;
            switch(err){
                case EAGAIN://资源暂时不可用，无数据或者缓冲区满
                case ECONNABORTED://对端进程崩溃或强制关闭连接
                case EINTR://系统调用被信号中断
                case EPROTO://协议错误，硬件/协议栈故障
                case EPERM://操作执行权限不足
                case EMFILE://进程打开文件描述符数量超过限制
                    errno = err;//把错误保留以供处理？
                    break;
                case EBADF://无效文件描述符
                case ENOTSOCK://文件描述符不是socket
                case EOPNOTSUPP://传入的socket不支持相应操作（accept）
                case EFAULT://内存错误，地址指向不可访问内存
                case EINVAL://参数无效

                case ENFILE://系统全局文件描述符耗尽
                case ENOBUFS://内核缓冲区不足，无法分配内核资源
                case ENOMEM://内存不足，无法分配内核数据结构
                    LOG_FATAL << "Unexpected error of ::accept: (" <<  err << ")" << strerror_tl(err);
                    break;
                default:
                    LOG_FATAL << "Unknow error of ::accept: (" <<  err << ")" << strerror_tl(err);
                    break;
            }
        }
        return connfd;
    }

    void shutDownWrite(int fd){
        if(::shutdown(fd, SHUT_WR) < 0){
            LOG_SYSERROR << "failed in shutDownWrite().";
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