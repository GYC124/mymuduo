#pragma once
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

class InetAddress { // 打包ip地址和端口号
public:
    // 该构造函数同样使用了 explicit 关键字，表示其不能进行隐式转换
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr)
        : addr_(addr)
    {}

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    const sockaddr_in* getSockAddr() const { return &addr_; }
    void setSockAddr(const sockaddr_in& addr) { addr_ = addr; }
private:
    sockaddr_in addr_;
};