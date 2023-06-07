#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

// 头文件中只写前置类型的声明，在源文件中实现,暴露的信息少
// 可以使用前置类型声明 (forward declaration) 来引用其它文件中的类而不必包含头文件
// 如果需要访问被声明类的成员函数或成员变量，则需要包含该类的头文件。
class EventLoop;

/*
 * Channel 类封装了文件描述符和其所关注的事件类型
 * Channel 理解为通道，封装了sockfd和其感兴趣的event, 如EPOLLIN、EPOLLOUT事件
 * 还绑定了poller返回的具体事件
*/
class Channel : noncopyable {
public:
    // std::function 是一个通用的、多态的函数封装类，能够封装任何可调用的目标，例如函数指针、成员函数指针、函数对象等。它可以用于实现回调函数、事件处理函数等。
    // EventCallback 表示一个事件回调函数，当该事件被触发时，会调用 EventCallback 对象所关联的函数来处理事件。由于该函数没有返回值，因此使用了 void 关键字；
    // 由于该函数没有参数，因此使用了空括号 () 表示参数列表为空。可以通过 std::function 的 operator() 来调用 EventCallback 对象所关联的函数。
    using EventCallback = std::function<void()>; // 无参数无返回值的函数对象
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    // fd得到poller的通知后，调用相应的回调函数处理事件
    void handleEvent(Timestamp receiveTime); // EventLoop使用可以前置声明，Timestamp不可以

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_  = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }

    // 防止当channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void>&); // 智能指针

    int fd() const { return fd_; }
    int events() const { return events_; } // 返回fd感兴趣的事件
    void set_revent(int revt) { revents_ = revt; } // poller监听fd本身发生的事件，提供对外接口设置

    // 设置fd相应的事件状态
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // one loop per thread
    EventLoop* ownerLoop() { return loop_; }
    void remove();
private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;  // 类中静态成员变量的定义和初始化是分开的，需要在类外部单独进行初始化

    EventLoop* loop_; // 事件循环
    const int fd_; // fd, Poller监听的对象
    int events_; // 注册fd感兴趣的事件
    int revents_; // poller返回的具体发生的事件
    int index_;

    std::weak_ptr<void> tie_; // 弱智能指针要监控强智能指针
    bool tied_;

    // 因为channel通道里面能够获知fd最终发生的具体事件revent_，所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_; // 类型为回调函数的变量
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};