#pragma once

#include <iostream>
#include <string>

class Timestamp {
public:
    Timestamp();
    /* C++中的explicit关键字只能用于修饰只有一个参数的类构造函数, 它的作用是
    *表明该构造函数是显示的, 而非隐式的, 跟它相对应的另一个关键字是implicit, 
    *意思是隐藏的,类构造函数默认情况下即声明为implicit(隐式).
    */
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp now();
    std::string tostring() const; // 被声明为 const，表示函数不会修改类成员变量
private:
    int64_t microSecondsSinceEpoch_;
};