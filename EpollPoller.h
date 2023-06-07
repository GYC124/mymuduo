#pragma once 

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Channel;

/*
 * epoll的使用
 * epoll_create
 * epoll_ctl    add/mod/del
 * epoll_wait
*/

class EpollPoller : public Poller {
public:
    EpollPoller(EventLoop* loop);
    ~EpollPoller() override; // 检测基类里面的虚函数，必须重写

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;
private:
    static const int kInitEventListSize = 16;

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    // 更新channel通道
    void update(int operation, Channel* channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_; // 构造函数使用epoll_create创建的epoll句柄（树根）
    EventList events_;
};

// // typedef union epoll_data
// {
//   void *ptr;
//   int fd;
//   uint32_t u32;
//   uint64_t u64;
// } epoll_data_t;

// struct epoll_event
// {
//   uint32_t events;	/* Epoll events */
//   epoll_data_t data;	/* User data variable */
// } __EPOLL_PACKED;