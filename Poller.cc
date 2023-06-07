#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop* loop) 
    : ownerLoop_(loop)
{
}

bool Poller::hasChannel(Channel *channel) const {
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

/*
 * 为什么不把newDefaultPoller静态方法写在Poller中？
 * newDefaultPoller当中要实现具体的IO复用并返回基类Poller的指针，则一定会包含#include "EpollPoller.h"
 * 才能创建一个newDefaultPoller的实例化对象并返回，而EpollPoller继承于Poller，基类可以引用派生类，派生类不能引用基类
*/ 
