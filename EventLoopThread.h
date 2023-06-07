#pragma once

#include "Thread.h"
#include "noncopyable.h"

#include <mutex>
#include <functional>
#include <string>
#include <condition_variable> // 条件变量头文件

/**
 * 条件变量是利用线程间共享的全局变量进行同步的一种机制，主要包括两个动作：一个线程等待 
 * 条件变量的条件成立而挂起;另一个线程使条件成立（给出条件成立信号）。为了防止竞争，条件变量的使用总是和一个互斥量结合在一起
*/ 

class EventLoop;

class EventLoopThread : noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(), 
        const std::string& name = std::string() );
    ~EventLoopThread();

    EventLoop* startLoop();
private:
    void threadFunc();

    EventLoop* loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};