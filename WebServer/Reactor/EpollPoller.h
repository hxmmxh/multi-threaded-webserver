#ifndef HXMMXH_EPOLLPOLLER_H
#define HXMMXH_EPOLLPOLLER_H

#include "Poller.h"

#include <vector>
/*
struct epoll_event {
    __uint32_t events; //Epoll events 
    epoll_data_t data; //User data variable 
};
*/
struct epoll_event; //提前声明，在#include <sys/epoll.h>中定义

namespace hxmmxh
{
class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize = 16;

    static const char *operationToString(int op);

    void fillActiveChannels(int numEvents,
                            ChannelList *activeChannels) const;
    void update(int operation, Channel *channel);

    typedef std::vector<struct epoll_event> EventList;

    int epollfd_;//
    EventList events_;//返回的已就绪事件集合
};
} // namespace hxmmxh

#endif