#pragma once 

#include <vector>
#include <unistd.h>
#include <string>

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode

// 网络库底层的缓冲器类型定义
class Buffer {
public:
    static const size_t kCheapPrepend = 8; // 预先准备的字节数
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    { }

    size_t readableBytes() const {
        return writerIndex_ - readerIndex_;
    }

    size_t writableBytes() const {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const {
        return readerIndex_;
    }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const {
        return begin() + readerIndex_;
    }

    // 函数的作用是将缓冲区读取指针往后移动 len 个字节。如果当前可读数据量大于 len，则只需要移动指针；
    // 否则说明缓冲区已经全部被读完，调用 retrieveAll() 函数将读写指针都置为初始状态。
    void retrieve(size_t len) { // // len就是应用程序从Buffer缓冲区读取的数据长度
        if(len < readableBytes()) {
            readerIndex_ += len;
        }
        else {
            retrieveAll();
        }
    }

    void retrieveAll() {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }


    /**
     * 这里的 std::string 构造函数的第一个参数是 const char* 类型的指针，它指向缓冲区中的某个字符。
     * 如果使用了 std::string(const char*) 这样的构造函数，那么 std::string 对象所包含的字符序列会
     * 一直延伸到遇到空字符（即 \0）。但是在这个例子中使用的是 std::string(const char*, size_t) 
     * 这样的构造函数，它会根据指定的长度 len 来截断字符串，也就是说不会读取超过 len 个字符。
    */
    std::string retrieveAsString(size_t len) {
        std::string result(peek(), len); // result 的值就是缓冲区中从 readerIndex_ 开始的长度为 len 的子串
        retrieve(len); // 上面一句把缓冲区中可读的数据，已经读取出来，这里肯定要对缓冲区进行复位操作
        return result;
    }

    // writableBytes() = buffer_.size() - writerIndex_
    void ensureWriteableBytes(size_t len) {
        if(writableBytes() < len) {
            makeSpace(len); // 扩容函数
        }
    }

    // 把[data, data+len]内存上的数据，添加到writable缓冲区当中
    void append(const char* data, size_t len) {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    char* beginWrite() {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const {
        return begin() + writerIndex_;
    }

    // 从fd上读取数据
    ssize_t readFd(int fd, int* saveErrno);
    // 从fd上发送数据
    ssize_t writeFd(int fd, int* saveErnno);
private:
    char* begin() {
        // it.operator*()
        return &*buffer_.begin(); // vector底层数组首元素的地址，也就是数组的起始地址
    }

    const char* begin() const {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len) {
        if(writableBytes() + prependableBytes() < len + kCheapPrepend) {
            buffer_.resize(writerIndex_ + len);
        }
        else {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_, 
                      begin() + kCheapPrepend);
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_; // 变量被用于表示缓冲区读取指针位置索引值
    size_t writerIndex_; // C中任何对象所能达到的最大长度，它是无符号整数,在数组下标和内存管理函数之类的地方广泛使用
};