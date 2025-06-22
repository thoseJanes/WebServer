#ifndef WEBSERVER_NET_TIMERQUEUE_H
#define WEBSERVER_NET_TIMERQUEUE_H
#include "timer.h"

#include <set>
#include <sys/timerfd.h>
#include <sys/socket.h>
#include "channel.h"
namespace webserver{

namespace detail{
int createTimerFd();

void readTimerFd(int fd);

void setTimerFd(int fd, TimeStamp timeStamp);

}

class EventLoop;
class TimerQueue:Noncopyable{//虽然功能还行，但感觉结构有点乱了。需要对比一下源码。另外，这样来看，好像这个类中的所有内容都是在EventLoop的线程中运行的？说是被EventLoop管理也不为过。
public:
    typedef std::pair<TimeStamp, Timer*> StampTimerPair;
    typedef std::pair<int, Timer*> IdTimerPair;

    TimerQueue(EventLoop* eventLoop)
    :   fd_(detail::createTimerFd()), 
        channel_(new Channel(fd_, eventLoop)), 
        handlingTimerCallback_(false)
    {
        channel_->setReadableCallback(std::bind(&TimerQueue::timerFdReadCallBack, this));
        channel_->enableReading();//更新到eventLoop中。但是如果此时poller还未生成呢？
    }
    ~TimerQueue(){
        channel_->disableAll();
        channel_->remove();
        ::close(fd_);
    }

    TimerId addTimer(TimeStamp start, function<void()> callback, double intervalSeconds){
        Timer* timer = new Timer(start, callback, intervalSeconds);
        insertTimer(timer);
        return TimerId(timer->getTimerId());
    }

    void insertTimer(Timer* timer){
        assert(timedTimers_.size() == numberedTimers_.size());
        assert(timedTimers_.find({timer->getTimeStamp(), timer})==timedTimers_.end());
        LOG_TRACE << "insert timer " << timer->getTimerId();
        timedTimers_.insert({timer->getTimeStamp(), timer});
        numberedTimers_.insert({timer->getTimerId(), timer});//集合会自动去重，即是重复插入也不需要担心。（resetTimers中可能重复插入）

        bool earliestChanged = (timedTimers_.begin()->second == timer);
        if(earliestChanged){
            LOG_TRACE << "in inserting timer "<<timer->getTimerId() << " earliestChanged to " <<timer->getTimerId();
            LOG_TRACE << "new timefd set at "<< timer->getTimeStamp().toFormattedString(true);
            resetTimerFd(timer);
        }
    }

    //未找到则返回false
    bool cancelTimer(TimerId id){
        int timerId = id.id();
        assert(timerId >= 0);
        channel_->assertInLoopThread();//可能在channel中，也可能在pendingFunctors。其实可以全部放进pendingFunctors中？但是这样，如果在执行channel时删除定时器，那么删除操作会被推迟。
        auto nTimerIt = numberedTimers_.lower_bound({timerId, reinterpret_cast<Timer*>(0)});
        if(nTimerIt==numberedTimers_.end() || nTimerIt->first != timerId){//在numberedTimers中寻找，如果没有找到，则一定是已经执行完了。
            LOG_ERROR << "Failed in cancelTimer(). Can't find timer " << timerId << ".";
            return false;
        }else{//找到了，可能刚开始执行，或者还没执行。
            if(handlingTimerCallback_){
                canceledTimers_.insert(timerId);
            }else{//可能定时器已经被执行完了。
                auto tTimerIt = timedTimers_.find({nTimerIt->second->getTimeStamp(), nTimerIt->second});
                bool earliestChanged = (timedTimers_.begin() == tTimerIt);

                delete tTimerIt->second;
                timedTimers_.erase(tTimerIt);
                numberedTimers_.erase(nTimerIt);
                assert(timedTimers_.size() == numberedTimers_.size());

                if(earliestChanged && !timedTimers_.empty()){
                    resetTimerFd(timedTimers_.begin()->second);
                }
            }
            return true;
        }
    }

private:






    void timerFdReadCallBack(){
        handlingTimerCallback_ = true;
        auto expiredTimers = getExpiredTimers();
        for(Timer* timer:expiredTimers){
            timer->run();//只有可能在这里有cancelTimer。
        }
        handlingTimerCallback_ = false;
        //在这里，cancelTimer已经执行完了，所以可以不用担心中途再向canceledTimers_插入id。
        restartTimers(expiredTimers);
        

        if(!timedTimers_.empty()){
            resetTimerFd(timedTimers_.begin()->second);
        }
    }

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

    void restartTimers(set<Timer*> timers){
        for(Timer* timer:timers){
            bool canceled = (canceledTimers_.find(timer->getTimerId())!=canceledTimers_.end());
            if(timer->isRepeat() && (!canceled)){
                timer->toNextTime();//reset后需要重新插入timedTimers_。
                insertTimer(timer);
            }else{
                auto nTimerIt = numberedTimers_.lower_bound({timer->getTimerId(), reinterpret_cast<Timer*>(0)});
                delete nTimerIt->second;
                numberedTimers_.erase(nTimerIt);
            }
        }
        canceledTimers_.clear();
    }

    void resetTimerFd(Timer* timer){
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