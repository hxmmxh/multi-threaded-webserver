#include "PollPoller.h"

#include "../../Log/Logging.h"
#include "Channel.h"

#include <cassert>
#include <errno.h>
#include <poll.h>

using namespace hxmmxh;

PollPoller::PollPoller(EventLoop *loop)
    : Poller(loop)
{
}

PollPoller::~PollPoller() = default;

Timestamp PollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    int numEvents = ::poll(pollfds_.data(), pollfds_.size(), timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LOG_TRACE << numEvents << " events happened";
        fillActiveChannels(numEvents, activeChannels);
    }
    else if (numEvents == 0)
        LOG_TRACE << " nothing happened";
    else
    {
        //被系统调用中断的错误忽略，其他错误记录下来
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_SYSERR << "PollPoller::poll()";
        }
    }

    return now;
}

void PollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (auto pfd = pollfds_.cbegin(); pfd != pollfds_.cend() && numEvents > 0; ++pfd)
    {
        if (pfd->revents > 0)
        {
            --numEvents;
            //在channels_这个map中找到fd对应的Channel，然后修改这个Channel
            auto ch = channels_.find(pfd->fd);
            assert(ch != channels_.end());
            Channel *channel = ch->second;
            assert(channel->fd() == pfd->fd);
            channel->set_revents(pfd->revents);
            activeChannels->push_back(channel);
        }
    }
}

//添加或更新
void PollPoller::updateChannel(Channel *channel)
{
    Poller::assertInLoopThread();
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
    if (channel->index() < 0)
    {
        // 未被某个Poller管理的Channel的index_是-1
        assert(channels_.find(channel->fd()) == channels_.end());
        struct pollfd pfd;
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        pollfds_.push_back(pfd);
        int idx = static_cast<int>(pollfds_.size() - 1);
        channel->set_index(idx);
        channels_[pfd.fd] = channel;
    }
    else
    {
        // 在这个poller中已存在，更新状态
        assert(channels_.find(channel->fd()) != channels_.end());
        assert(channels_[channel->fd()] == channel);
        int idx = channel->index();
        assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
        struct pollfd &pfd = pollfds_[idx];
        //fd相等，fd被设为-fd-1代表着不关心这个文件描述符上的事件
        assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        //如果这个channel不关心任何事件，则把fd设置为channel->fd()的相反数减一
        //只是在pollfds_数组中修改，在ChannelMap中fd作为关键字，保持原样
        if (channel->isNoneEvent())
            pfd.fd = -channel->fd() - 1;
    }
}

void PollPoller::removeChannel(Channel *channel)
{
    Poller::assertInLoopThread();
    LOG_TRACE << "fd = " << channel->fd();
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    //channel没有关心的事件时才能被删除
    assert(channel->isNoneEvent());
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    const struct pollfd &pfd = pollfds_[idx];
    (void)pfd;
    ////channel没有关心的事件时才能被删除
    assert(pfd.fd == -channel->fd() - 1 && pfd.events == channel->events());
    //从Channel map中删除，复杂度为O(log N)，返回被移除的元素数。
    size_t n = channels_.erase(channel->fd());
    assert(n == 1);
    (void)n;
    //从pollfds_数组中移除
    if (static_cast<size_t>(idx) == pollfds_.size() - 1)
    {
        pollfds_.pop_back();
    }
    else
    {
        int channelAtEnd = pollfds_.back().fd;
        //交换两个指针指向的内容
        iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
        //找回原来的文件描述符
        if (channelAtEnd < 0)
        {
            channelAtEnd = -channelAtEnd - 1;
        }
        channels_[channelAtEnd]->set_index(idx);
        pollfds_.pop_back();
    }
}
