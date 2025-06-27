#include "epollPoller.h"


#include "../event/channel.h"
#include "../event/eventLoop.h"

using namespace mynetlib;

static_assert(POLLOUT == EPOLLOUT, "event flags of poll and epoll not equal.");
static_assert(POLLIN == EPOLLIN, "event flags of poll and epoll not equal.");
static_assert(POLLPRI == EPOLLPRI, "event flags of poll and epoll not equal.");
static_assert(POLLHUP == EPOLLHUP, "event flags of poll and epoll not equal.");
static_assert(POLLRDHUP == EPOLLRDHUP, "event flags of poll and epoll not equal.");
static_assert(POLLERR == EPOLLERR, "event flags of poll and epoll not equal.");
//static_assert(POLLNVAL == , "event flags of poll and epoll not equal."); epoll没有POLLNVAL吗？为什么？


EpollPoller::EpollPoller(EventLoop* loop):pollFd_(epoll_create(4096)), loop_(loop){
    events_.resize(16);
}


EpollPoller::~EpollPoller(){
    ::close(pollFd_);
}

std::vector<Channel*> EpollPoller::poll(){
    //loop_->assertInLoopThread();
    std::vector<Channel*> channels;
    int n = epoll_wait(pollFd_, events_.data(), events_.size(), -1);
    for(int i=0;i<n;i++){
        auto event = events_[i];
        Channel* channel = channelsMap_[event.data.fd];
        channel->setREvent(event.events);
        channels.push_back(channel);
    }
    if(n == events_.size()){
        events_.resize(2*n);
    }
    return channels;
}


void EpollPoller::updateChannel(Channel* channel){
    if(channel->getPollState() == Channel::sNone || channel->getPollState() == Channel::sDeleted){
        if(channelsMap_.find(channel->getFd()) != channelsMap_.end()){
            LOG_ERROR << "Conflict fd in channelsMap.";
        }
        channelsMap_.insert({channel->getFd(), channel});
        channel->setPollState(Channel::sDisabled);
    }

    assert(channelsMap_.find(channel->getFd()) != channelsMap_.end());
    assert(channelsMap_.find(channel->getFd())->second == channel);

    if(channel->getPollState() == Channel::sDisabled){
        if(channel->hasEvent()){
            epoll_event event = eventOfChannel(channel);
            epoll_ctl(pollFd_, EPOLL_CTL_ADD, channel->getFd(), &event);
            channel->setPollState(Channel::sActive);
        }
    }else if(channel->getPollState() == Channel::sActive){
        if(channel->hasEvent()){
            epoll_event event = eventOfChannel(channel);
            epoll_ctl(pollFd_, EPOLL_CTL_MOD, channel->getFd(), &event);
        }else{
            epoll_ctl(pollFd_, EPOLL_CTL_DEL, channel->getFd(), NULL);
            channel->setPollState(Channel::sDisabled);
        }
    }
}

void EpollPoller::removeChannel(Channel* channel){
    channelsMap_.erase(channel->getFd());
    if(channel->getPollState() == Channel::sActive){
        epoll_ctl(pollFd_, EPOLL_CTL_DEL, channel->getFd(), NULL);
    }else if(channel->getPollState() != Channel::sDisabled){
        LOG_FATAL << "Failed in removeChannel(). Channel not in map.";
    }
    channel->setPollState(Channel::sDeleted);
}

epoll_event EpollPoller::eventOfChannel(Channel* channel){
    epoll_event event; memset(&event, 0, sizeof(epoll_event));
    event.data.fd = channel->getFd();
    event.events = channel->getEvent();
    return event;
}

bool EpollPoller::hasChannel(Channel* channel){
    return (channelsMap_.find(channel->getFd())!=channelsMap_.end());
}



