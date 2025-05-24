#include "channel.h"
#include "eventLoop.h"

using namespace webserver;


void Channel::update(){
    loop_->updateChannel(this);
}


void Channel::assertInLoopThread(){
    loop_->assertInLoopThread();
}