#include "channel.h"
#include "eventLoop.h"

using namespace mynetlib;


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

Channel::~Channel(){
    assert(state_ == sNone || state_ == sDeleted);//为什么不在析构函数中将channelremove出poller？
    //assert(loop_->hasChannel(this) == false);
    assert(!handlingEvents_);
}