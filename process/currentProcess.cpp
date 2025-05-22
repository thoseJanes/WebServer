#include "currentProcess.h"
#include "../common/format.h"
#include "../fileUtil/fileUtil.h"
#include <limits>
#include <pwd.h>

namespace webserver{

int p_pid = 0;
char p_pidString[32];
int p_pidStringLen = 6;

namespace CurrentProcess{
static_assert(std::is_same<pid_t, int>::value, "pid_t should be int");


string procname(){
    SmallFileReader statContent("/proc/self/stat");
    string_view content = statContent.toStringView();
    auto p1 = content.find_first_of("(") +1;
    auto len = content.find_first_of(")", p1-1)-p1;
    if(0 < len){
        auto nameView = content.substr(p1, len);
        return string(nameView);
    }
    return "";
}

string username(){
    char buf[128];
    if(0 == gethostname(buf, sizeof(buf))){
        return buf;
    }
    return "";
}

string hostname(){
    char buf[512];
    passwd pwd;
    passwd* result;
    int ret = getpwuid_r(uid(), &pwd, buf, sizeof(buf), &result);
    if(0 == ret){
        return pwd.pw_name;
    }else{
        return "";
    }
}

}


}

