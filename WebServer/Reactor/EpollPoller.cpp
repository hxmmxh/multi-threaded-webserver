#include "EpollPoller.h"
#include "../../Log/Logging.h"
#include "Channel.h"

#include <cassert>
#include <cerrno>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

using namespace hxmmxh;

//Linux系统中，两者定义应该一致
static_assert(EPOLLIN == POLLIN, "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI, "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT, "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP, "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR, "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP, "epoll uses same flag values as poll");

namespace
{
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
} // namespace

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_SYSFATAL << "EPollPoller::EPollPoller";
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    LOG_TRACE << "fd total count " << channels_.size();
    int numEvents = ::epoll_wait(epollfd_,
                                 &*events_.begin(),
                                 static_cast<int>(events_.size()),
                                 timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LOG_TRACE << numEvents << " events happened";
        //epoll返回的是已经就绪的事件集合，可以直接使用
        //poll返回所有的事件，是否就绪还需要检测revent值
        fillActiveChannels(numEvents, activeChannels);
        //扩大容量
        if (static_cast<size_t>(numEvents) == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_TRACE << "nothing happened";
    }
    else
    {
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_SYSERR << "EPollPoller::poll()";
        }
    }
    return now;
}
/*
typedef union epoll_data {
    void *ptr;
    int fd;
    __uint32_t u32;
    __uint64_t u64;
} epoll_data_t;

struct epoll_event {
    __uint32_t events; //Epoll events 
    epoll_data_t data; //User data variable 
};
*/
void EPollPoller::fillActiveChannels(int numEvents,
                                         ChannelList *activeChannels) const
{
    assert(static_cast<size_t>(numEvents) <= events_.size());
    for (int i = 0; i < numEvents; ++i)
    {
        //ptr是一个void*类型指针，在添加事件时将它设置成Channel*指针
        //所以这里可以用static_cast取回
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        int fd = channel->fd();
        ChannelMap::const_iterator it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void EPollPoller::updateChannel(Channel *channel)
{
    Poller::assertInLoopThread();
    const int index = channel->index();
    LOG_TRACE << "fd = " << channel->fd()
              << " events = " << channel->events() << " index = " << index;
    //没有类似PollPoller中的pollfd_数组，所以Channel的idx简单的分为3类
    if (index == kNew || index == kDeleted)
    {
        // a new one, add with EPOLL_CTL_ADD
        int fd = channel->fd();
        if (index == kNew)
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        else // index == kDeleted
        {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        // update existing one with EPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        (void)fd;
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel *channel)
{
    Poller::assertInLoopThread();
    int fd = channel->fd();
    LOG_TRACE << "fd = " << fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());
    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    size_t n = channels_.erase(fd);
    (void)n;
    assert(n == 1);

    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EPollPoller::update(int operation, Channel* channel)
{
  struct epoll_event event;
  memset(&event,0, sizeof event);
  event.events = channel->events();
  //在这里把event.data的void*指针设为Channel*
  event.data.ptr = channel;
  int fd = channel->fd();
  LOG_TRACE << "epoll_ctl op = " << operationToString(operation)
    << " fd = " << fd << " event = { " << channel->eventsToString() << " }";
  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
  {
    if (operation == EPOLL_CTL_DEL)
    {
      LOG_SYSERR << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
    }
    else
    {
      LOG_SYSFATAL << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
    }
  }
}

const char* EPollPoller::operationToString(int op)
{
  switch (op)
  {
    case EPOLL_CTL_ADD:
      return "ADD";
    case EPOLL_CTL_DEL:
      return "DEL";
    case EPOLL_CTL_MOD:
      return "MOD";
    default:
      assert(false && "ERROR op");
      return "Unknown Operation";
  }
}
