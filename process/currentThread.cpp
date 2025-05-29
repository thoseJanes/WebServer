#include "currentThread.h"
#include <type_traits>
#include <pthread.h>
#include <unistd.h>//syscall


namespace webserver{

namespace CurrentThread{

    
__thread int t_tid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLen = 6;//为什么等于6？
__thread const char* t_threadName = "unknow";
static_assert(std::is_same<pid_t, int>::value, "pid_t should be int");



__thread char t_errBuf[512];


}

const char* strerror_tl(int err){
    return strerror_r(err, CurrentThread::t_errBuf, sizeof CurrentThread::t_errBuf);
}




}
