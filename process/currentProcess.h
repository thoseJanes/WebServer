#ifndef WEBSERVER_PROCESS_PROCESS_H
#define WEBSERVER_PROCESS_PROCESS_H


#include <unistd.h>
#include <string>
#include <linux/utsname.h>
#include <limits.h>
#include <./common/format.h>

namespace webserver{

extern int p_pid;
extern char p_pidString[32];
extern int p_pidStringLen;

namespace currentProcess{
using namespace std;

inline pid_t pid(){
    if(__builtin_expect(p_pid==0, 0)){
        cachePid();
    }
    return p_pid;
}

inline const char* pidString(){
    if(__builtin_expect(p_pid==0, 0)){
        cachePid();
    }
    return p_pidString;
}

inline int pidStringLen(){
    if(__builtin_expect(p_pid==0, 0)){
        cachePid();
    }
    return p_pidStringLen;
}

inline pid_t cachePid(){
    p_pid = getpid();
    char buf[detail::maxFormatSize<pid_t>()];
    p_pidStringLen = detail::formatInteger<pid_t>(buf, pid());
    buf[p_pidStringLen] = '\0';
}


uid_t uid(){
    return getuid();
}
uid_t euid(){
    return geteuid();
}



}

}

#endif