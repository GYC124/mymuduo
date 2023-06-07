#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

// EventLoop: ChannelList Poller
Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{}

Channel::~Channel() {}

// channel的tie方法什么时候调用? TcpConnection新连接创建时 TcpConnection => Channel
void Channel::tie(const std::shared_ptr<void>& obj) {
    tie_ = obj;
    tied_ = true;
}

/*
 *当改变channel所表示fd的events事件后，update负责在poller里面更改相应的事件epoll_ctl
 *EventLoop =>ChannelList Poller
*/
void Channel::update() {
    // 通过channel所属的EventLoop，调用poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}

// 在当前channel所属的EventLoop中，移除当前的channel
void Channel::remove() {
    loop_->removeChannel(this);
}

// fd得到poller通知以后，处理事件的
void Channel::handleEvent(Timestamp receiveTime) {
    if(tied_) {
        std::shared_ptr<void> guard = tie_.lock(); // lock将弱智能指针强化为强智能指针
        if(guard) {
            handleEventWithGuard(receiveTime);
        }
    }
    else {
        handleEventWithGuard(receiveTime);
    }
}

// 根据poller通知的channel发生的具体事件，由channel负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime) {
    LOG_INFO("channel handleEvent revent : %d\n", revents_);

    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) { // EPOLLHUP (挂起)表示读写都关闭
        if(closeCallback_) {
            closeCallback_();
        }
    }

    if(revents_ & (EPOLLERR)) { // 按位与 同1为1
        if(errorCallback_) {
            errorCallback_();
        }
    }

    if(revents_ & (EPOLLIN | EPOLLPRI)) {
        if(readCallback_) {
            readCallback_(receiveTime);
        }
    }

    /*
     *writeCallback_ 是一个 EventCallback 类型的成员变量，它表示当有可写事件发生时需要执行的回调函数。
     *如果该成员变量不为空，说明已经注册了回调函数，那么就执行该回调函数。执行回调函数的方式是通过执行函数对象 writeCallback_() 来实现，
     *因为 writeCallback_ 是一个 std::function 对象，可以像函数一样执行它。该回调函数是无参数和无返回值的，因为 EventCallback 的定义就是一个无参数和无返回值的 std::function 对象。
     *这种通过函数指针或函数对象来实现回调操作的方式，是一种常见的设计模式，称为回调模式或观察者模式。利用回调模式可以实现代码解耦，降低代码复杂度，提高程序扩展性和维护性。
    */
    if(revents_ & EPOLLOUT) {
        if(writeCallback_) {
            writeCallback_();
        }
    }
}