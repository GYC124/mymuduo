#pragma once

/*
 *noncopyable被继承之后，派生类对象可以正常的构造和析构，但是派生类对象
 *无法进行拷贝构造和赋值操作
*/

class noncopyable {
public:
    noncopyable(const noncopyable&) = delete; // 拷贝构造
    void operator=(const noncopyable&) = delete; // 赋值
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};