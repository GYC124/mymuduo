#include "Timestamp.h"

#include "time.h"

Timestamp::Timestamp() : microSecondsSinceEpoch_(0) {}
Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch)
    {}
Timestamp Timestamp::now() {
    return Timestamp(time(NULL)); // 返回值是从1900年1月1日至今所经历的时间（以秒为单位）
}
std::string Timestamp::tostring() const {
    char buf[128] = {0};
    tm* tm_time = localtime(&microSecondsSinceEpoch_);
    snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d",
            tm_time->tm_year + 1900,
            tm_time->tm_mon + 1,
            tm_time->tm_mday,
            tm_time->tm_hour,
            tm_time->tm_min,
            tm_time->tm_sec);
    return buf;
}

// #include <iostream>
// int main() {
//     std::cout << Timestamp::now().tostring() << std::endl;
//     return 0;
// }