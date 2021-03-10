#ifndef HXMMXH_POLLPOLLER_H
#define HXMMXH_POLLPOLLER_H

#include "Poller.h"

#include <vector>

struct pollfd; //定义在<poll.h>中，提前声明
/*
struct pollfd {
int fd;              //监视的描述符
short events;        //该描述符上监视的事件
short revents;       //该描述符上发生的事件
};
*/
namespace hxmmxh
{
class PollPoller : public Poller
{
public:
    PollPoller(EventLoop *loop);
    ~PollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    // 把已经就绪的事件存入到activeChannels中
    void fillActiveChannels(int numEvents,
                            ChannelList *activeChannels) const;

    typedef std::vector<struct pollfd> PollFdList;
    PollFdList pollfds_;
};
} // namespace hxmmxh

#endif