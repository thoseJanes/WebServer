#ifndef WEBSERVER_THREAD_CURRENTTHREAD_H
#define WEBSERVER_THREAD_CURRENTTHREAD_H
#include <string.h>//strerror_r
#include <sys/syscall.h>//SYS_gettid
#include <stdio.h>
#include <unistd.h>

namespace webserver{

const char* strerror_tl(int err);

namespace CurrentThread{

extern __thread int t_tid;
extern __thread char t_tidString[32];
extern __thread int t_tidStringLen;
extern __thread const char* t_threadName;

inline void cacheTid(){
    if(!t_tid){
        t_tid = syscall(SYS_gettid);
        t_tidStringLen = snprintf(t_tidString, sizeof t_tidString, "%5d", t_tid);//最小宽度5,不足部分用空格填充
    }
}

inline int tid(){
    if(__builtin_expect(t_tid==0, 0)){
        cacheTid();
    }
    return t_tid;
}

inline const char* tidString(){
    if(__builtin_expect(t_tid==0, 0)){
        cacheTid();
    }
    return t_tidString;
}

inline int tidStringLen(){
    if(__builtin_expect(t_tid==0, 0)){
        cacheTid();
    }
    return t_tidStringLen;
}


}


}

#endif