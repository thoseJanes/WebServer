#include "epollPoller.h"

#include <sys/epoll.h>
#include "../net/channel.h"
#include "../net/eventLoop.h"

using namespace webserver;

EpollPoller::EpollPoller(EventLoop* loop):pollFd_(epoll_create(4096)), loop_(loop){
    events_.resize(16);
}


std::vector<Channel*> EpollPoller::poll(){
    loop_->assertInLoopThread();
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
    if(channel->getPollState() == Channel::None || channel->getPollState() == Channel::Deleted){
        LOG_ERR << "Conflict fd in channelsMap.";
        channelsMap_.insert({channel->getFd(), channel});
        channel->setPollState(Channel::Disabled);
    }

    assert(channelsMap_.find(channel->getFd()) != channelsMap_.end());
    assert(channelsMap_.find(channel->getFd())->second == channel);

    if(channel->getPollState() == Channel::Disabled){
        if(channel->hasEvent()){
            epoll_event event = eventOfChannel(channel);
            epoll_ctl(pollFd_, EPOLL_CTL_ADD, channel->getFd(), &event);
            channel->setPollState(Channel::Active);
        }
    }else if(channel->getPollState() == Channel::Active){
        if(channel->hasEvent()){
            epoll_event event = eventOfChannel(channel);
            epoll_ctl(pollFd_, EPOLL_CTL_MOD, channel->getFd(), &event);
        }else{
            epoll_ctl(pollFd_, EPOLL_CTL_DEL, channel->getFd(), NULL);
            channel->setPollState(Channel::Disabled);
        }
    }
}

void EpollPoller::removeChannel(Channel* channel){
    channelsMap_.erase(channel->getFd());
    if(channel->getPollState() == Channel::Active){
        epoll_ctl(pollFd_, EPOLL_CTL_DEL, channel->getFd(), NULL);
    }else if(channel->getPollState() != Channel::Disabled){
        LOG_FATAL << "Failed in removeChannel(). Channel not in map.";
    }
    channel->setPollState(Channel::Deleted);
}

epoll_event EpollPoller::eventOfChannel(Channel* channel){
    epoll_event event; memset(&event, 0, sizeof(epoll_event));
    event.data.fd = channel->getFd();
    event.events = channel->getEvent();
    return event;
}





