#pragma once

#include "noncopyable.h"

#include <thread> // c++11提供的线程类操作
#include <functional>
#include <memory>
#include <string>
#include <atomic>

class Thread : noncopyable {
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string& name = std::string());
    ~Thread();

    // start()、join()、tid()，用于启动线程、等待线程结束和获取线程 ID
    void start();
    void join();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string& name() const { return name_; }

    static int numCreated() { return numCreated_; }
private:
    void setDefaultName();

    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_; // 存储线程函数
    std::string name_;

    // 用于原子操作一个整数类型的变量。它提供了一组接口，使得对于该变量的读写操作是原子的，即不会被中断或其他线程干扰，保证了多线程编程时的正确性和可靠性。
    static std::atomic_int numCreated_; // 记录产生线程的个数
};