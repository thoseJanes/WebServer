#ifndef WEBSERVER_NET_POLLER_H
#define WEBSERVER_NET_POLLER_H

#include <vector>
#include <map>
namespace webserver{

class Channel;
class Poller{
public:
    Poller();
    ~Poller();
    virtual std::vector<Channel*> poll() = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;//通过文件描述符找到需要删除的channel。
private:
    std::map<int, Channel*> channels_;

}

    
};
#endif