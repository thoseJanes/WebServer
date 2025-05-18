#include "currentProcess.h"
#include "../common/format.h"
#include <limits>


namespace webserver{

int p_pid = 0;
char p_pidString[32];
int p_pidStringLen = 6;

namespace CurrentProcess{
static_assert(std::is_same<pid_t, int>::value, "pid_t should be int");


string procname(){
    return "";
}
string username(){
    return "";
}
string hostname(){
    return "";
}

}


}

