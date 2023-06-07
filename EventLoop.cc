#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

// 防止一个线程创建多个EventLoop thread_local
__thread EventLoop* t_loopInThisThread = nullptr;
// __thread 是 GCC/Clang 中用来标记线程局部存储的关键字，它可以将变量声明为每个线程独有的副本。在多线程编程中，
// 使用 __thread 可以避免同一变量被多个线程共享而导致的线程安全问题。每个线程都拥有自己独立的变量副本，这样一来，即使多个线程同时访问同一个变量，也不会相互影响

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd，用来notify唤醒subReactor处理新来的channel
int createEventfd() {
    // eventfd 是一个 Linux 特有的系统调用，用于创建一个 eventfd 文件描述符。该文件描述符具有可读性和可写性，
    // 可以用于在多个线程或进程之间传递计数器值或事件信号。第一个参数表示计数器的初始值，第二个参数 flags 可以
    // 用来设置一些额外的属性，例如本代码中设置的 EFD_NONBLOCK 表示将文件描述符设置为非阻塞模式，EFD_CLOEXEC 表示在执行 execve 系统调用时关闭文件描述符。
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0) {
        LOG_FATAL("evtfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop() 
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this)) // poller_ 对象是 Poller 类型的指针，它指向 Poller 的派生类对象，在构造函数中将其初始化为 EpollPoller 对象。
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop create %p in thread %d \n", this, threadId_);
    if(t_loopInThisThread) {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个eventloop都将监听wakeupchannel的EPOLLIN读事件
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

// 开启事件循环
void EventLoop::loop() {
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while(!quit_) {
        activeChannels_.clear();
        // 监听两类fd  一种client的fd  一种wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for(Channel* channel : activeChannels_) {
            // Poller监听哪些channel发生事件，然后上报给EventLoop，通知channel处理相应事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作
        /**
         * IO线程 mainLoop accept fd <= channel subloop
         * mainLoop 事先注册一个回调cb（需要subloop来执行） wakeup subloop后，执行下面的方法，执行之前mainloop注册的cb操作
        */
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

// 退出事件循环 1.loop在自己的线程中调用quit  2.在非loop的线程中，调用loop的quit
void EventLoop::quit() {
    quit_ = true;

    // 如果是在其它线程中，调用的quit   在一个subloop(woker)中，调用了mainLoop(IO)的quit
    if(!isInLoopThread()) {
        wakeup();
    }
}

// 在当前loop中执行cb
void EventLoop::runInLoop(Functor cb) {
    if(isInLoopThread()) { // 在当前loop中执行cb
        cb();
    }
    else { // 在非当前loop线程中执行cb，就要唤醒loop线程，执行cb
        queueInLoop(cb);
    }
}

// 把cb放入队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb) {
    { // 因为需要并发执行，就要添加锁
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb); // emplace_back直接构造，push为拷贝构造
    }

    // 唤醒相应的，需要执行上面回调操作的loop的线程了
    // || callingPendingFunctors_的意思是：当前loop正在执行回调，但是loop又有了新的回调,处理完doPendingFunctors()后，阻塞在poll，然后被唤醒，继续执行回调
    if(!isInLoopThread() || callingPendingFunctors_) {
        wakeup(); // 唤醒loop所在线程
    }
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if(n != sizeof one) {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
    }
}

// 用来唤醒loop所在线程 向wakeupfd_写一个数据，wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if(n != sizeof(one)) {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

// EventLoop的方法 => Poller的方法
void EventLoop::updateChannel(Channel* channel) {
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel) {
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors() { // 执行回调
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor& functor : functors) {
        functor(); // 执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}