#pragma once
#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class EventLoop;
class Channel;

// muduo库中多路事件分发器的核心IO复用模块
class Poller : noncopyable {
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller() = default;

    // 给所有IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    // 判断参数channel是否在当前Poller当中
    bool hasChannel(Channel* channel) const;

    // EventLoop可以通过该接口获取默认的IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop* loop);
private:
    EventLoop* ownerLoop_; // 定义Poller所属的事件循环EventLoop
protected:
    // map的key: sockfd   value: sockfd所属的channel通道类型
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_; // 负责记录 文件描述符 ---> Channel的映射，也帮忙保管所有注册在你这个Poller上的Channel,添加到epoll树，channel中有事件发生，则填写活跃连接
};