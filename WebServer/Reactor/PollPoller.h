#ifndef HXMMXH_POLLPOLLER_H
#define HXMMXH_POLLPOLLER_H

#include "Poller.h"

#include <vector>

struct pollfd; //定义在<poll.h>中，提前声明

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
    void fillActiveChannels(int numEvents,
                            ChannelList *activeChannels) const;

    typedef std::vector<struct pollfd> PollFdList;
    PollFdList pollfds_;
};
} // namespace hxmmxh

#endif