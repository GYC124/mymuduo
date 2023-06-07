#include "InetAddress.h"

#include "string.h"

//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

InetAddress::InetAddress(uint16_t port, std::string ip) {
    bzero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port); // 主机字节序转换为网络字节序
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

std::string InetAddress::toIp() const {
    // addr_取出ip转换为主机字节序
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}

std::string InetAddress::toIpPort() const {
    // ip : port
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf + end, ": %u", port);
    return buf;
}

uint16_t InetAddress::toPort() const {
    return ntohs(addr_.sin_port);
}

// #include <iostream>

// int main() {
//     InetAddress addr(8080);
//     std::cout << addr.toIp() << std::endl;
//     return 0;
// }