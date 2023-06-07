#include "Logger.h"
#include "Timestamp.h"

#include <iostream>

// 使用单例模式创建唯一对象
Logger& Logger::instance() {
    static Logger instance_;
    return instance_;
}
// 设置日志级别
void Logger::setLogLevel(int Level) {
    logLevel_ = Level;
}
// 写日志 [级别信息] time : msg
void Logger::log(std::string msg) {
    switch (logLevel_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FATAL:
        std::cout << "[FATAL]";
        break;
    case DEBUG:
        std::cout << "[DEBUG]";
        break;
    default:
        break;
    }

    // 打印时间和msg
    std::cout << Timestamp::now().tostring() << ":" << msg << std::endl;
}