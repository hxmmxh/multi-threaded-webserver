#include "Logging.h"
#include "Channel.h"
#include "EventLoop.h"

#include <poll.h>
#include <sstream>

using namespace hxmmxh;

const int Channel::kNoneEvent = 0;
// 可读事件
const int Channel::kReadEvent = POLLIN | POLLPRI;
// 可写事件
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fd__)
    : loop_(loop),
      fd_(fd__),
      events_(0),
      revents_(0),
      index_(-1),
      logHup_(true),
      tied_(false),
      eventHandling_(false),
      addedToLoop_(false)
{
}

//析构时，保证EventLoop
Channel::~Channel()
{
    assert(!eventHandling_);
    assert(!addedToLoop_);
    if (loop_->isInLoopThread())
    {
        assert(!loop_->hasChannel(this));
    }
}

void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

//加入到所属EventLoopd的Poller的Channel库中
void Channel::update()
{
    addedToLoop_ = true;
    loop_->updateChannel(this);
}

void Channel::remove()
{
    assert(isNoneEvent());
    addedToLoop_ = false;
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    std::shared_ptr<void> guard;
    if (tied_)
    {
        // weak_ptr::lock()
        // 如果与tie共享对象的shared_ptr的数量为0，则返回一个空指针
        // 否则返回一个指向w的对象的shared_ptr
        guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

/*
*POLLIN: 普通话或优先级带数据可读
*POLLPRI：高优先级数据可读
*POLLRDHUP: 对端关闭
--------------------------------------
*POLLOUT： 普通数据和优先级数据可写
---------------------------------------
*POLLLNVAL: 描述符不是一个打开的文件
*POLLERR： 发生错误
*POLLHUP: 发生挂起,socket另一端关闭
*/

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    // 开始处理事件
    eventHandling_ = true;
    // 记录事件的名字
    LOG_TRACE << reventsToString();
    //POLLHUP: 对端关闭，且没有读操作，就关闭
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
    {
        if (logHup_)
        {
            LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
        }
        if (closeCallback_)
            closeCallback_();
    }
    // 描述符不是一个打开的文件
    if (revents_ & POLLNVAL)
    {
        LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLNVAL";
    }
    // 发生错误
    if (revents_ & (POLLERR | POLLNVAL))
    {
        if (errorCallback_)
            errorCallback_();
    }
    // 可写
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
    {
        if (readCallback_)
            readCallback_(receiveTime);
    }
    // 可读
    if (revents_ & POLLOUT)
    {
        if (writeCallback_)
            writeCallback_();
    }
    // 停止处理事件
    eventHandling_ = false;
}

string Channel::reventsToString() const
{
    return eventsToString(fd_, revents_);
}

string Channel::eventsToString() const
{
    return eventsToString(fd_, events_);
}

string Channel::eventsToString(int fd, int ev)
{
    std::ostringstream oss;
    oss << fd << ": ";
    if (ev & POLLIN) //普通或优先级带数据可读
        oss << "IN ";
    if (ev & POLLPRI)// 高优先级数据可读
        oss << "PRI ";
    if (ev & POLLOUT)// 普通数据和优先级数据可写
        oss << "OUT ";
    if (ev & POLLHUP)//发生挂起,socket另一端关闭
        oss << "HUP ";
    if (ev & POLLRDHUP)//对端关闭
        oss << "RDHUP ";
    if (ev & POLLERR)//发生错误
        oss << "ERR ";
    if (ev & POLLNVAL)//描述符不是一个打开的文件
        oss << "NVAL ";

    return oss.str();
}