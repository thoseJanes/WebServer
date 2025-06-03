#include "currentThread.h"
#include <type_traits>
#include <pthread.h>
#include <unistd.h>//syscall
#include <execinfo.h>
#include <cxxabi.h>


namespace webserver{

namespace detail{

void afterFork(){
    CurrentThread::t_tid = 0;
    CurrentThread::tid();
}

class ThreadInitializer{
public:
    ThreadInitializer(){
        pthread_atfork(NULL, NULL, &detail::afterFork);
    }
};

ThreadInitializer threadInitializer;
}


namespace CurrentThread{

    
__thread int t_tid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLen = 6;//为什么等于6？
__thread const char* t_threadName = "unknow";
static_assert(std::is_same<pid_t, int>::value, "pid_t should be int");

__thread char t_errBuf[512];

/*
如果是g++编译，需要指定-g -rdynamic两个选项

等效于在CMakeList中加上：

    # 设置编译器标志以生成调试信息
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

    # 设置链接器标志以加载动态符号表
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
*/
std::string stackTrace(bool demangle){
    int maxFrameSize = 200;
    void* frames[maxFrameSize];
    int frameSize = backtrace(frames, maxFrameSize);
    char** stacks = backtrace_symbols(frames, frameSize);

    size_t initSize = 256;
    char* demangledBuf = demangle?static_cast<char*>(malloc(initSize*sizeof(char))):nullptr;
    std::string stackString;
    for(int i=0;i<frameSize;i++){
        if(demangle){
            char* funcStart = strchr(stacks[i], '(');
            char* funcEnd = strchr(funcStart, '+');
            if(!funcStart || !funcEnd){continue;}
            
            funcStart++;
            *funcEnd = '\0';
            int status = 0;
            char* buf = abi::__cxa_demangle(funcStart, demangledBuf, &initSize, &status);
            *funcEnd = '+';
            if(status != 0){continue;}

            demangledBuf = buf;
            stackString.append(stacks[i], funcStart);
            stackString.append(buf);
            stackString.append(funcEnd);
            stackString.push_back('\n');
        }else{
            stackString.append(stacks[i]);
            stackString.push_back('\n');
        }
    }
    free(demangledBuf);
    free(stacks);
    return stackString;
}

}

const char* strerror_tl(int err){
    return strerror_r(err, CurrentThread::t_errBuf, sizeof CurrentThread::t_errBuf);
}




}
