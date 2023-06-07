#include "EpollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <cstring>

// channel未添加到poller中
const int kNew = -1; // channel的成员index_ = -1;
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;

EpollPoller::EpollPoller(EventLoop* loop) 
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC)) // ::可以明确告诉编译器要寻找的符号位于全局命名空间中，而不是当前命名空间中
    , events_(kInitEventListSize) // vector<epoll_event>
{
    if(epollfd_ < 0) {
        LOG_FATAL("epoll_create error: %d\n", errno);
    }
}

EpollPoller::~EpollPoller() {
    ::close(epollfd_);
}

// 在 epoll_wait() 函数调用期间，内核会扫描 epoll 实例所监听的所有文件描述符，并将其中发生事件的文件描述符加入就绪列表中，同时返回就绪列表中的文件描述符个数。
// 就绪列表本质上是一个数组或 vector，其每个元素对应一个已经准备好的文件描述符。
Timestamp EpollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    LOG_INFO("func=%s => fd total count:%lu \n", __FUNCTION__, channels_.size());
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs); 
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if(numEvents > 0) {
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if(numEvents == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    }
    else if(numEvents == 0) { // 超时
        LOG_DEBUG("%s timeout! \n", __FUNCTION);
    }
    else { // 发生错误
        if(saveErrno != EINTR) {
            errno = saveErrno;
            LOG_ERROR("EpollPoller::poll() err!");
        }
    }
    return now;
}

// updateChannel和removeChannel操作对应epoll_ctl
// channel update remove => EventLoop updateChannel removeChannel => Poller
/**
 *          / ChannelList
 * EventLoop        
 *          \ Poller ChannelMap  <fd, channel*>
*/                  
void EpollPoller::updateChannel(Channel* channel) { 
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);

    if(index == kNew || index == kDeleted) {
        if(index == kNew) {
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel); // 添加节点
    }
    else { // channel已经在poller注册过 
        int fd = channel->fd();
        if(channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else { // channel中有事件
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从Poller中删除channel
void EpollPoller::removeChannel(Channel* channel) {
    int fd = channel->fd();
    channels_.erase(fd);
    
    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);

    int index = channel->index();
    if(index == kDeleted) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
} 

// 填写活跃连接
void EpollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const {
    for(int i = 0; i < numEvents; i++) {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revent(events_[i].events);
        activeChannels->push_back(channel); // 至此EventLoop拿到了它的poller给它返回的所有发生的channel列表
    }
}

// 更新channel通道 epoll_ctl add/mod/del
void EpollPoller::update(int operation, Channel* channel) {
    epoll_event event;
    memset(&event, 0, sizeof(event));
    int fd = channel->fd();

    event.data.fd = fd;
    event.events = channel->events();
    event.data.ptr = channel; // ptr中存放fd相关的参数比如channel

    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        if(operation == EPOLL_CTL_DEL) {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}