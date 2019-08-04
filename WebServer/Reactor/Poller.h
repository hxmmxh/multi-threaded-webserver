#ifndef HXMMXH_POLLER_H
#define HXMMXH_POLLER_H

#include <vector>
#include <map>

#include "EventLoop.h"
#include "../Time/Timestamp.h"

namespace hxmmxh
{
class Channel;

//纯虚类，IO复用函数可用poll或epoll实现
class Poller
{
public:
    typedef std::vector<Channel *> ChannelList;

    Poller(EventLoop *loop);
    virtual ~Poller();

    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;

    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;
    virtual bool hasChannel(Channel *channel) const;

    static Poller *newDefaultPoller(EventLoop *loop);

    void assertInLoopThread() const { ownerLoop_->assertInLoopThread(); }

protected:
    typedef std::map<int, Channel *> ChannelMap;
    ChannelMap channels_;

private:
    EventLoop *ownerLoop_;
};
} // namespace hxmmxh
#endif