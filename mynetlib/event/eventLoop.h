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
    EventLoop();
    
    ~EventLoop();
    
    void loop();

    void quit();

    void runInLoop(function<void()> func);
    void queueInLoop(function<void()> func);
    int getSizeOfPendingFunctions();


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


    TimerId runAt(TimeStamp start, function<void()> callback);
    TimerId runAfter(int msecond, function<void()> callback);
    TimerId runEvery(TimeStamp start, function<void()> callback, double intervalSeconds);
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
    void runPendingFunctions();

    void wakeupCallback();

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