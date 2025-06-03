#include <signal.h>
#include <unistd.h>
#include "eventLoop.h"



using namespace webserver;
__thread EventLoop* EventLoop::t_eventLoop_ = NULL;

namespace{


//忽略sigpipe信号，防止意外写导致的程序退出。
class IgnoreSigPipe{
public:
IgnoreSigPipe(){
    ::signal(SIGPIPE, SIG_IGN);
}
};
IgnoreSigPipe initSigPipe;

}
