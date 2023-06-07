#include "Thread.h"
#include "CurrentThread.h"

#include "semaphore.h"

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string& name) 
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func)) // 右值引用
    , name_(name)
{
    setDefaultName();
}

Thread::~Thread() {
    if(started_ && !joined_) {
        thread_->detach();// thread类提供的设置分离线程的方法
        // 守护线程（主线程结束，守护线程自动结束）
    }
}

/**
 * 该方法利用 C++11 中的 std::thread 和 POSIX 标准的信号量机制实现了一个
 * 简单的线程创建和同步过程。其中，std::thread 提供了跨平台的线程创建和管理接口，
 * 而 POSIX 信号量则提供了线程间同步的功能。
*/
void Thread::start() { // 一个Thread对象，记录的就是一个新线程的详细信息
    started_ = true;
    sem_t sem; // 信号量
    sem_init(&sem, false, 0);

    // 开启线程, 启动一个新线程，并在新线程中执行指定的线程函数,线程和线程的调度没有顺序而言
    thread_ = std::shared_ptr<std::thread>(new std::thread([&] () {
        // 获取线程的tid值, 并通过调用 sem_post 使主线程可以继续往下执行；
        tid_ = CurrentThread::tid();
        sem_post(&sem); // sem_post() 函数会将指定的信号量的值加 1，并唤醒因等待该信号量而被阻塞的线程。应该在调用 sem_init() 函数成功初始化信号量之后使用。在多线程编程中，
                        // 通常需要使用 sem_post() 函数来保证对共享资源的访问和修改的互斥性和同步性，从而避免竞态条件、死锁等问题。
        // 开启一个新线程， 专门执行该线程函数
        func_();
    })); //  lambda 表达式作为线程函数

    // 这里必须等待获取上面新创建的线程的tid值
    sem_wait(&sem);
}

void Thread::join() {
    joined_ = true;
    thread_->join(); 
}

void Thread::setDefaultName() {
    int num = ++numCreated_; // 按照创建的序号，起默认的名字
    if(name_.empty()) {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }
}