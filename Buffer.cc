#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/**
 * 从fd上读取数据  Poller工作在LT模式
 * Buffer缓冲区是有大小的！ 但是从fd上读数据的时候，却不知道tcp数据最终的大小
 */ 
// 从fd上读取数据
ssize_t Buffer::readFd(int fd, int* saveErrno) {
    char extrabuf[65536] = {0}; // 栈上的内存空间 64K

    struct iovec vec[2];

    const size_t writable = writableBytes(); // 这是Buffer底层缓冲区剩余的可写空间大小
    vec[0].iov_base = begin() + writerIndex_; // 第一片缓冲区的起始地址
    vec[0].iov_len = writable; // 缓冲区的长度大小

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt); // readv可以根据读出的字段自动填写缓冲区
    if(n < 0) {
        *saveErrno = errno;
    }
    else if(n <= writable) { // Buffer的可缓冲区已经够存储读出来的数据
        writerIndex_ += n;
    }
    else { // extrabuf里面也写入了数据
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}   

// 从fd上发送数据
ssize_t Buffer::writeFd(int fd, int* saveErnno) {
    ssize_t n = ::write(fd, peek(), readableBytes());
    if(n < 0) {
        *saveErnno = errno;
    }
    return n;
}