
#include <unistd.h>
#include <string>
#include <linux/utsname.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <string.h>//strerror_r
#include <pwd.h>//getpwuid_r
#include <string>
#include "../../mynetbase/common/fileUtil.h"
#include "../../mynetbase/process/currentProcess.h"
#include <sys/prctl.h>
using namespace std;
using namespace mynetlib;

int main(){
    prctl(PR_SET_NAME, "123456789abcdefghijklmnopqrst");
    char bufprctl[128];
    prctl(PR_GET_NAME, bufprctl);
    printf("PR_GET_NAME:%s\n", bufprctl);
    
    
    string name;
    name = CurrentProcess::procname();
    printf("procName: %s\n", name.c_str());

    name = CurrentProcess::username();
    printf("userName: %s\n", name.c_str());

    name = CurrentProcess::hostname();
    printf("hostName: %s\n", name.c_str());

    char buf[512];
    int ret  = gethostname(buf, sizeof(buf));
    if(ret == 0){
        printf("%s\n", buf);
    }else if(ret == -1){
        int err = errno;
        strerror_r(err, buf, sizeof buf);
        printf("get host name failed, %s\n", buf);
    }
    
    passwd pwd;
    passwd* result = new passwd;
    ret = getpwuid_r(getuid(), &pwd, buf, sizeof buf, &result);
    if(0 == ret){
        printf("%s\n", pwd.pw_name);
    }else if(ERANGE == ret){    
        printf("get pwuid failed, buf size is not enough.\n");
    }
}