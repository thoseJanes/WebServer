#include <signal.h>
#include <unistd.h>
#include "eventLoop.h"



using namespace mynetlib;
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


EventLoop::EventLoop()
:   wakeupFd_(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)), 
    wakeupChannel_(new Channel(wakeupFd_, this)),
    handlingChannels_(false), 
    handlingPendingFuncs_(false),
    threadTid_(CurrentThread::tid()), 
    timerQueue_(new TimerQueue(this)),
    poller_(new EpollPoller(this))
{
    if(t_eventLoop_){
        LOG_FATAL << "Failed in EventLoop construct. This thread already has an EventLoop object.";
    }
    t_eventLoop_ = this;
    wakeupChannel_->setReadableCallback(bind(&EventLoop::wakeupCallback, this));
    wakeupChannel_->enableReading();
    IgnoreSigPipe initSigPipe;//未加该句时仍会报SIGPIPE错误，增加后不再报错。
}


EventLoop::~EventLoop(){
    assert(!running_);
    wakeupChannel_->disableAll();//为什么要先disable再remove？为什么不直接remove？
    wakeupChannel_->remove();
    ::close(wakeupFd_);

    t_eventLoop_ = NULL;
}

void EventLoop::loop(){
    assertInLoopThread();
    running_ = true;
    while(running_){
        std::vector<Channel*> channels = std::move(poller_->poll());
        handlingChannels_ = true;//暂时没用。
        for(auto channel:channels){
            assert(channel->hasEvent());
            channel->run();
        }
        handlingChannels_ = false;
        
        handlingPendingFuncs_ = true;//暂时没用。
        runPendingFunctions();
        handlingPendingFuncs_ = false;
    }
    LOG_DEBUG << "loop over";
}

void EventLoop::quit(){
    assert(running_);
    if(running_){
        running_ = false;
        if(!isInLoopThread()){
            wakeup();
        }
    }
}

void EventLoop::runInLoop(function<void()> func){
    if(isInLoopThread()){
        func();
    }else{
        queueInLoop(std::move(func));
    }
}

void EventLoop::queueInLoop(function<void()> func){
    MutexLockGuard lock(mutex_);
    pendingFuncs_.push_back(func);
    if(!handlingChannels_){
        LOG_TRACE << "eventLoop in thread " << threadTid_ << " on waking up";
        wakeup();
    }
}
int EventLoop::getSizeOfPendingFunctions(){
    int size;
    {
        MutexLockGuard lock(mutex_);
        size = pendingFuncs_.size();
    }
    return size;
}


void EventLoop::runPendingFunctions(){//是否需要状态相关的判断？
    assertInLoopThread();
    vector<function<void()>> callingFuncs;
    {
        MutexLockGuard lock(mutex_);
        callingFuncs = std::move(pendingFuncs_);
    }
    for(auto func:callingFuncs){
        func();
    }
}


void EventLoop::wakeupCallback(){
    //assertInLoopThread();这个assert会在channel中保证，这里不需要担心它。
    LOG_TRACE << "eventLoop in thread " << threadTid_ << " responses waking up";
    int64_t buf;
    int n = read(wakeupFd_, &buf, sizeof(buf));
    if(n != 8){
        LOG_SYSERROR << "Expect 8 bytes but read " << n << " bytes in EventLoop::readWakeupFd().";
    }
}

TimerId EventLoop::runAt(TimeStamp start, function<void()> callback){
    Timer* timer = new Timer(start, callback);
    runInLoop(bind(&TimerQueue::insertTimer, timerQueue_.get(), timer));
    return TimerId(timer->getTimerId());
}
TimerId EventLoop::runAfter(int msecond, function<void()> callback){
    TimeStamp start = TimeStamp::now();
    TimeStamp now = start;
    start.add(msecond*1000);
    
    assert(start.getMicroSecondsSinceEpoch() - now.getMicroSecondsSinceEpoch() == msecond*1000);
    Timer* timer = new Timer(start, callback);
    LOG_TRACE << "timer "<< timer->getTimerId() <<" msecond*1000:" << start.getMicroSecondsSinceEpoch() - now.getMicroSecondsSinceEpoch();
    runInLoop(bind(&TimerQueue::insertTimer, timerQueue_.get(), timer));
    return TimerId(timer->getTimerId());
}
TimerId EventLoop::runEvery(TimeStamp start, function<void()> callback, double intervalSeconds){
    //assertInLoopThread();
    Timer* timer = new Timer(start, callback, intervalSeconds);
    runInLoop(bind(&TimerQueue::insertTimer, timerQueue_.get(), timer));
    return TimerId(timer->getTimerId());
}