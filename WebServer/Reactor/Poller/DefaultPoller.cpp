#include "EpollPoller.h"
#include "PollPoller.h"
#include "../Poller.h"

#include <stdlib.h>

using namespace hxmmxh;

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
  if (::getenv("MUDUO_USE_POLL"))
  {
    return new PollPoller(loop);
  }
  else
  {
    return new EPollPoller(loop);
  }
}
