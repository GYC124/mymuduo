#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, 
    const std::string& name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name)
    , mutex_()
    , cond_()
    , callback_(cb)
{
}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if(loop_ != nullptr) {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop() {
    thread_.start(); // 启动底层thread的新线程

    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_ == nullptr) {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

// TODO 剖析one loop per thread：调用startLoop时底层函数才创建线程，同时构造函数参数创建一个事件循环
// 下面的方法是在单独的新线程里面运行
void EventLoopThread::threadFunc() {
    EventLoop loop; // 创建一个单独的eventloop，和上面的线程是一一对应的，one loop per thread

    if(callback_) {
        callback_(&loop); // TcpServer中传来的线程初始化回调
    }

    { // std::condition_variable 对象通常使用 std::unique_lock<std::mutex> 来等待
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one(); // cond_.notify_one() 通常和 cond_.wait() 对应使用
    }

    loop.loop(); // EventLoop loop -> Poller.poll
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}