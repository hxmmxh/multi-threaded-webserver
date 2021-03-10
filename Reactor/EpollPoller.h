#ifndef HXMMXH_EPOLLPOLLER_H
#define HXMMXH_EPOLLPOLLER_H

#include "Poller.h"

#include <vector>
/*
struct epoll_event {
    __uint32_t events; //Epoll events 
    epoll_data_t data; //User data variable 
};
typedef union epoll_data {
    void *ptr;
    int fd;
    __uint32_t u32;
    __uint64_t u64;
} epoll_data_t;
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
    // 一开始已就绪事件数组的大小
    // 如果遇到epoll后它满了的情况，大小会增加一倍
    static const int kInitEventListSize = 16;
    static const char *operationToString(int op);

    void fillActiveChannels(int numEvents,
                            ChannelList *activeChannels) const;
    void update(int operation, Channel *channel);

    
    int epollfd_;//epoll是有文件描述符的
    typedef std::vector<struct epoll_event> EventList;
    EventList events_;//返回的已就绪事件集合
};
} // namespace hxmmxh

#endif