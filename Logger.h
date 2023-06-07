#pragma once

#include <string>

#include "noncopyable.h"
/* 程序调用类中方法只有两种方式，
 *①创建类的一个对象，用该对象去调用类中方法；
 *②使用类名直接调用类中方法，格式“类名::方法名()”
*/

// LOG_INFO("%s %d", arg1, arg2)
// 这段代码是一个宏定义，它实现了在程序中方便地输出日志信息的功能。
// 在代码中调用 LOG_INFO(format, ...) 时，执行的实际操作包括设置日志级别、格式化日志消息和将日志消息写入日志文件等步骤。
#define LOG_INFO(logmsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)

#define LOG_ERROR(logmsgFormat, ...) \
do { \
    Logger &logger = Logger::instance(); \
    logger.setLogLevel(ERROR); \
    char buf[1024] = {0}; \
    snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
    logger.log(buf); \
} while(0)

#define LOG_FATAL(logmsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    } while(0)

#ifdef MUDEBUG // debug输出信息太多，需要时再使用
#define LOG_DEBUG(logmsgFormat, ...) \
    do { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)
#else
    #define LOG_DEBUG(LogmsgFormat, ...)
#endif

enum LogLevel {
    INFO, // 普通信息 0
    ERROR, // 错误信息 1
    FATAL, // 崩溃信息core 2
    DEBUG, //调试信息 3
};

class Logger : noncopyable {
public:
    /*static Logger& instance() 是一个静态成员函数，用于创建并返回 Logger 类型的静态对象的引用。
     *它被声明为静态成员函数是因为单例模式要求在程序整个运行过程中只存在一个 Logger 实例，因此需要通过静态方法来获取唯一实例。
    */
    // 使用单例模式创建唯一对象
    static Logger& instance();
    // 设置日志级别
    void setLogLevel(int Level);
    // 写日志
    void log(std::string msg);
private:
    // 单例模式将构造函数私有化，禁止其他程序创建该类的对象，因此自己要创建一个供程序使用
    Logger() {}
    int logLevel_;
};