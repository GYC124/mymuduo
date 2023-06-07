#pragma once

#include "noncopyable.h"

class InetAddress;

class Socket : noncopyable {
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {}
    ~Socket();

    int fd() const { return sockfd_; } // 只读接口，写为const
    void bindAddress(const InetAddress& localaddr);
    void listen();
    int accept(InetAddress* peeraddr);

    void shutdownWrite(); // 关闭写端

    // 更改TCP选项
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};