#ifndef HXMMXH_CHANNEL_H
#define HXMMXH_CHANNEL_H

#include "../../Time/Timestamp.h"

#include <functional>
#include <memory>
#include <string>

namespace hxmmxh
{

class EventLoop; //Channel里保存着一个EventLoop指针

class Channel
{
public:
    typedef std::function<void()> EventCallback;
    typedef std::function<void(Timestamp)> ReadEventCallback;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    void handleEvent(Timestamp receivetime);
    void setReadCallback(ReadEventCallback cb)
    {
        readCallback_ = std::move(cb);
    }
    void setWriteCallback(EventCallback cb)
    {
        writeCallback_ = std::move(cb);
    }
    void setErrorCallback(EventCallback cb)
    {
        errorCallback_ = std::move(cb);
    }
    void setCloseCallback(EventCallback cb)
    {
        closeCallback_ = std::move(cb);
    }

    /// Tie this channel to the owner object managed by shared_ptr,
    /// prevent the owner object being destroyed in handleEvent.
    void tie(const std::shared_ptr<void> &);

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; }
    bool isNoneEvent() const { return events_ == kNoneEvent; }

    //设置关心的事件
    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }
    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }
    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }
    //是否在监听可写或可读信号
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    std::string reventsToString() const;
    std::string eventsToString() const;

    void doNotLogHup() { logHup_ = false; }

    EventLoop *ownerLoop() { return loop_; }
    void remove();

private:
    static std::string eventsToString(int fd, int ev);
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;
    const int fd_;
    //下面两个成员来自于poll函数的struct pollfd
    int events_;  //表示关心的IO事件
    int revents_; //目前活动的事件
    int index_;   //在Poller中的序号
    bool logHup_; //是否记录POLLHUP事件

    std::weak_ptr<void> tie_;
    bool tied_;
    bool eventHandling_; //是否正在处理事件
    bool addedToLoop_;   //是否属于某个EventLoop对象
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};

} // namespace hxmmxh

#endif