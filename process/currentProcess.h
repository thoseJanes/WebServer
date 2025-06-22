#ifndef WEBSERVER_PROCESS_PROCESS_H
#define WEBSERVER_PROCESS_PROCESS_H


#include <unistd.h>
#include <string>
#include <linux/utsname.h>
#include <limits.h>
#include <assert.h>

#include "../common/format.h"

namespace webserver{

extern int p_pid;
extern char p_pidString[32];
extern int p_pidStringLen;

namespace CurrentProcess{
using namespace std;

inline void cachePid(){
    p_pid = getpid();
    p_pidStringLen = detail::formatInteger<pid_t>(p_pidString, p_pid);
    assert(p_pidStringLen < sizeof(p_pidString));
    p_pidString[p_pidStringLen] = '\0';
}

inline pid_t pid(){
    if(__builtin_expect(0==p_pid, 0)){
        cachePid();
    }
    return p_pid;
}

inline const char* pidString(){
    if(__builtin_expect(0==p_pid, 0)){
        cachePid();
    }
    return p_pidString;
}

inline int pidStringLen(){
    if(__builtin_expect(0==p_pid, 0)){
        cachePid();
    }
    return p_pidStringLen;
}


inline uid_t uid(){
    return getuid();
}
inline uid_t euid(){
    return geteuid();
}

string procname();
string username();
string hostname();

}

}

#endif