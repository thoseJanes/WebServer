#include "channel.h"
#include "eventLoop.h"

using namespace webserver;


int Channel::kReadingEvent = POLLPRI | POLLIN;
int Channel::kWritingEvent = POLLOUT;
int Channel::kNoneEvent = 0;//(~POLLOUT) & (~(POLLPRI | POLLIN));//这样会开启所有其它事件

void Channel::update(){
    loop_->updateChannel(this);
}
void Channel::remove(){
    loop_->removeChannel(this);
    assert(!loop_->hasChannel(this));
}


void Channel::assertInLoopThread(){
    loop_->assertInLoopThread();
}