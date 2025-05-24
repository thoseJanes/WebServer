#ifndef WEBSERVER_NET_TIMERQUEUE_H
#define WEBSERVER_NET_TIMERQUEUE_H
#include "timer.h"

#include <set>
#include <sys/timerfd.h>
#include "channel.h"
namespace webserver{

namespace detail{
int createTimerFd(){
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK | TFD_TIMER_ABSTIME);
    if(fd < 0){
        LOG_FATAL << "failed in createTimerFd";
    }
}

int readTimerFd(int fd){
    int64_t ovt;
    int n = ::read(fd, &ovt, sizeof(ovt));
    if(n != 8){
        LOG_ERR << "expect 8 bytes but read " << n << " bytes in readTimerFd()!";
    }
}

int setTimerFd(int fd, TimeStamp timeStamp){//setTimerfd时，是否需要取消之前设置的timerfd？
    auto microSeconds = timeStamp.getMicroSecondsSinceEpoch() - TimeStamp::now().getMicroSecondsSinceEpoch();
    if(microSeconds < 0){
        LOG_WARN << "microSeconds "<< microSeconds <<" is negative in setTimerFd()!";
    }
    itimerspec t; memset(&t, 0, sizeof(t));
    t.it_value.tv_sec = microSeconds/TimeStamp::kMicroSecondsPerSecond;
    t.it_value.tv_nsec = microSeconds%TimeStamp::kMicroSecondsPerSecond*1000;//微秒到纳秒的转换。
    
    if(timerfd_settime(fd, NULL, &t, NULL)){
        LOG_FATAL << "failed in timerfd_settime().";
    }
}

}

class EventLoop;
class TimerQueue{//虽然功能还行，但感觉结构有点乱了。需要对比一下源码。另外，这样来看，好像这个类中的所有内容都是在EventLoop的线程中运行的？说是被EventLoop管理也不为过。
public:
    typedef std::pair<TimeStamp, Timer*> StampTimerPair;
    typedef std::pair<int, Timer*> IdTimerPair;

    TimerQueue(EventLoop* eventLoop):fd_(detail::createTimerFd()), channel_(new Channel(fd_, eventLoop)), handlingTimerCallback_(false){
        channel_->enableReadableEvent(std::bind(timerFdReadCallBack, this));
        channel_->update();//更新到eventLoop中。但是如果此时poller还未生成呢？
    }
    ~TimerQueue();

    void addTimer(Timer* timer){
        //channel_->assertInLoopThread();
        
        insert(timer);
        //return timer->getTimerId();
        //timedTimers_.insert({time, timer});
    }

    int cancelTimer(int timerId){
        channel_->assertInLoopThread();
        auto nTimerIt = numberedTimers_.lower_bound({timerId, reinterpret_cast<Timer*>(0)});
        if(nTimerIt==numberedTimers_.end() || nTimerIt->first != timerId){
            if(handlingTimerCallback_){//如果没有在执行回调，那么应该是能找到的？否则就是timer已经执行完毕了。即使在执行回调，用的也是timedTimers_？
                canceledTimers_.insert(timerId);
            }else{
                LOG_ERR << "Failed in cancelTimer(). Can't find timer " << timerId << ".";
            }
        }else{
            auto tTimerIt = timedTimers_.find({nTimerIt->second->getTimeStamp(), nTimerIt->second});
            delete tTimerIt->second;
            timedTimers_.erase(tTimerIt);
            numberedTimers_.erase(nTimerIt);
        }
    }

private:
    std::set<Timer*> getExpiredTimers(){
        StampTimerPair bound = {TimeStamp::now(), reinterpret_cast<Timer*>(std::numeric_limits<ptrdiff_t>::max())};
        auto boundIt = timedTimers_.lower_bound(bound);
        assert(boundIt == timedTimers_.end() || boundIt->first > bound.first);

        std::set<Timer*> timers;
        for(auto it = timedTimers_.begin();it != boundIt;it++){
            timers.insert((*it).second);
        }
        timedTimers_.erase(timedTimers_.begin(), boundIt);
        return timers;
    }

    void insert(Timer* timer){
        assert(timedTimers_.find({timer->getTimeStamp(), timer})==timedTimers_.end());
        timedTimers_.insert({timer->getTimeStamp(), timer});
        numberedTimers_.insert({timer->getTimerId(), timer});//集合会自动去重，即是重复插入也不需要担心。（resetTimers中可能重复插入）

        bool earliestChanged = (timedTimers_.begin()->second == timer);
        if(earliestChanged){
            setTimerFd(timer);
        }
    }

    int timerFdReadCallBack(){
        channel_->assertInLoopThread();
        handlingTimerCallback_ = true;
        auto expiredTimers = getExpiredTimers();
        for(Timer* timer:expiredTimers){
            timer->run();//只有可能在这里有cancelTimer。
        }
        handlingTimerCallback_ = false;
        //在这里，cancelTimer已经执行完了，所以可以不用担心中途再向canceledTimers_插入id。
        resetTimers(expiredTimers);
    }    

    void resetTimers(set<Timer*> timers){
        for(Timer* timer:timers){
            if(timer->isRepeat() && canceledTimers_.find(timer->getTimerId())==canceledTimers_.end()){
                timer->reset();//reset后需要重新插入timedTimers_。
                insert(timer);
            }else{
                auto nTimerIt = numberedTimers_.lower_bound({timer->getTimerId(), reinterpret_cast<Timer*>(0)});
                delete nTimerIt->second;
                numberedTimers_.erase(nTimerIt);
            }
        }
        canceledTimers_.clear();
    }


    void setTimerFd(Timer* timer){
        detail::setTimerFd(fd_, timer->getTimeStamp());
    }

    int fd_;
    unique_ptr<Channel> channel_;
    std::set<StampTimerPair> timedTimers_;//通过TimeStamp比较的Timer？将最小的放在最上面。
    std::set<IdTimerPair> numberedTimers_;//用于索引进行删除。
    std::set<int> canceledTimers_;
    //EventLoop* loop_;
    bool handlingTimerCallback_;
};

}

#endif