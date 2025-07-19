#ifndef MYNETLIB_POLLER_EPOLLPOLLER_H
#define MYNETLIB_POLLER_EPOLLPOLLER_H
#include "../event/poller.h"

#include <sys/epoll.h>
#include "../../mynetbase/common/patterns.h"
namespace mynetlib{

class EpollPoller:Noncopyable, public Poller{
public:
    EpollPoller(EventLoop* loop);
    ~EpollPoller();
    std::vector<Channel*> poll() override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;//通过文件描述符找到需要删除的channel。
    bool hasChannel(Channel* channel) override;
    
private:
    epoll_event eventOfChannel(Channel* channel);
    int pollFd_;
    EventLoop* loop_;
    std::vector<epoll_event> events_;
    std::map<int, Channel*> channelsMap_;
};    


}



#endif