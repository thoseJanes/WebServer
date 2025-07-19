#ifndef MYNETLIB_NET_EVENTLOOP_H
#define MYNETLIB_NET_EVENTLOOP_H



#include "channel.h"
#include "timer.h"
#include "timerQueue.h"
#include "poller.h"
#include <memory>
#include "../../mynetbase/common/mutexLock.h"
#include "../poller/epollPoller.h"
#include "sys/eventfd.h"
using namespace std;
namespace mynetlib{


class Poller;
class EventLoop:Noncopyable{
public:
    EventLoop()
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
    }
    
    ~EventLoop(){
        assert(!running_);
        wakeupChannel_->disableAll();//为什么要先disable再remove？为什么不直接remove？
        wakeupChannel_->remove();
        ::close(wakeupFd_);

        t_eventLoop_ = NULL;
    }
    
    void loop(){
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

    void quit(){
        assert(running_);
        if(running_){
            running_ = false;
            if(!isInLoopThread()){
                wakeup();
            }
        }
    }


    void runInLoop(function<void()> func){
        if(isInLoopThread()){
            func();
        }else{
            queueInLoop(std::move(func));
        }
    }
    void queueInLoop(function<void()> func){
        MutexLockGuard lock(mutex_);
        pendingFuncs_.push_back(func);
        if(!handlingChannels_){
            LOG_TRACE << "eventLoop in thread " << threadTid_ << " on waking up";
            wakeup();
        }
        
    }
    int getSizeOfPendingFunctions(){
        int size;
        {
            MutexLockGuard lock(mutex_);
            size = pendingFuncs_.size();
        }
        return size;
    }


    void updateChannel(Channel* channel){
        runInLoop(bind(&Poller::updateChannel, poller_.get(), channel));
        //assertInLoopThread();
        //poller_->updateChannel(channel);
    }
    void removeChannel(Channel* channel){
        runInLoop(bind(&Poller::removeChannel, poller_.get(), channel));
        // assertInLoopThread();
        // //assert()是否应该允许在handlingChannels_时从poller删除channel？
        // poller_->removeChannel(channel);
    }
    bool hasChannel(Channel* channel){
        assertInLoopThread();
        return poller_->hasChannel(channel);
    }


    TimerId runAt(TimeStamp start, function<void()> callback){
        Timer* timer = new Timer(start, callback);
        runInLoop(bind(&TimerQueue::insertTimer, timerQueue_.get(), timer));
        return TimerId(timer->getTimerId());
    }
    TimerId runAfter(int msecond, function<void()> callback){
        TimeStamp start = TimeStamp::now();
        TimeStamp now = start;
        start.add(msecond*1000);
        
        assert(start.getMicroSecondsSinceEpoch() - now.getMicroSecondsSinceEpoch() == msecond*1000);
        Timer* timer = new Timer(start, callback);
        LOG_TRACE << "timer "<< timer->getTimerId() <<" msecond*1000:" << start.getMicroSecondsSinceEpoch() - now.getMicroSecondsSinceEpoch();
        runInLoop(bind(&TimerQueue::insertTimer, timerQueue_.get(), timer));
        return TimerId(timer->getTimerId());
    }
    TimerId runEvery(TimeStamp start, function<void()> callback, double intervalSeconds){
        //assertInLoopThread();
        Timer* timer = new Timer(start, callback, intervalSeconds);
        runInLoop(bind(&TimerQueue::insertTimer, timerQueue_.get(), timer));
        return TimerId(timer->getTimerId());
    }
    void cancelTimer(TimerId timerId){
        //assertInLoopThread();
        runInLoop(bind(&TimerQueue::cancelTimer, timerQueue_.get(), timerId));//只能尽力而为地删除。
    }


    void assertInLoopThread(){//需要打印调用堆栈。需要用宏定义打印__FILE__和__LINE__吗？
        if(!isInLoopThread()){
            LOG_FATAL << "Function is in thread "<< CurrentThread::tidString() << " but expected in thread " << threadTid_;
        }
    }
    void assertInChannelHandling(){
        if(!isInLoopThread()){
            LOG_FATAL << "Function is in thread "<< CurrentThread::tidString() << " but expected in thread " << threadTid_;
        }else if(!handlingChannels_){
            LOG_FATAL << "Function is in right thread " << threadTid_ << " but not handling channels";
        }
    }
    void assertInPendingFunctors(){
        if(!isInLoopThread()){
            LOG_FATAL << "Function is in thread "<< CurrentThread::tidString() << " but expected in thread " << threadTid_;
        }else if(!handlingPendingFuncs_){
            LOG_FATAL << "Function is in right thread " << threadTid_ << " but not handling pending functors";
        }
    }
    bool isInLoopThread(){
        return threadTid_ == CurrentThread::tid();
    }
    static EventLoop* getThreadEventLoop(){
        return t_eventLoop_;
    }

    void wakeup(){
        //即使是在自己线程，也需要wakeup。不过由于readWakeupFd也是在自己线程，所以，可能刚wakeup，然后就被readWakeupFd读取走了？
        //但是这也没关系，因为wakeup就是为了唤醒epoll_wait而存在的。
        int64_t buf = 1;
        int n = ::write(wakeupFd_, &buf, sizeof(buf));
        if(n != 8){
            LOG_SYSERROR << "Expect 8 bytes but write " << n << " bytes in EventLoop::wakeup().";
        }
    }

    int getThreadId(){
        return CurrentThread::tid();
    }
private:
    void runPendingFunctions(){//是否需要状态相关的判断？
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


    void wakeupCallback(){
        //assertInLoopThread();这个assert会在channel中保证，这里不需要担心它。
        LOG_TRACE << "eventLoop in thread " << threadTid_ << " responses waking up";
        int64_t buf;
        int n = read(wakeupFd_, &buf, sizeof(buf));
        if(n != 8){
            LOG_SYSERROR << "Expect 8 bytes but read " << n << " bytes in EventLoop::readWakeupFd().";
        }
    }


    static __thread EventLoop* t_eventLoop_;
    pid_t threadTid_;

    bool running_;
    bool handlingChannels_;
    bool handlingPendingFuncs_;

    unique_ptr<Poller> poller_;//先有poller，才能够更新timer的channel
    unique_ptr<TimerQueue> timerQueue_;
    MutexLock mutex_;//用于保护timerQueue_
    std::vector<function<void()>> pendingFuncs_;
    
    int wakeupFd_;
    unique_ptr<Channel> wakeupChannel_;
};







}
#endif