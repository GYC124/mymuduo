#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>

class EventLoop;
class Channel;
class Socket;

/**
 * TcpConnction打包成功连接客户端的通信链路
 * TcpServer => Acceptor => 有一个新用户连接，通过accept函数拿到connfd
 * => TcpConnction 设置回调 => Channel => Poller => Channel
*/

// std::enable_shared_from_this<TcpConnection>能够返回一个shared_ptr指针，由于shared_ptr是可自动释放内存的智能指针，它会管理TcpConnection对象的生命周期，
// 保证在TcpConnection对象不再被使用时及时释放。同时，返回的shared_ptr指针可以延长TcpConnection对象的生命周期，防止被提前释放掉。
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(EventLoop* loop, 
                  const std::string& nameAge,
                  int sockfd,
                  const InetAddress& localAddr, // 主机ip和端口
                  const InetAddress& peerAddr); // 客户端ip和端口
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; } // 只读函数，不能修改变量成员
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }

    // 发送数据
    void send(const std::string& buf);
    // 关闭连接
    void shutdown();

    void setConnectionCallback(const ConnectionCallback& cb) {
        connectionCallback_ = cb;
    }

    void setMessageCallback(const MessageCallback& cb) {
        messageCallback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
        writeCompleteCallback_ = cb;
    }

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }

    void setCloseCallback(const CloseCallback& cb) {
        closeCallback_ = cb;
    }

    // 连接建立
    void connectEstablished();
    // 连接销毁
    void connectDestroyed();
private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
    void setState(StateE state) { state_ = state; }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();

    EventLoop* loop_; // 此处不是baseLoop，因为TcpConnection都是在subLoop里面管理的
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    // 这里和Acceptor类似   Acceptor=》mainLoop    TcpConenction=》subLoop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_; // 有新连接时的回调
    MessageCallback messageCallback_; // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成以后的回调
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_;

    Buffer inputBuffer_; // 接收缓冲区
    Buffer outputBuffer_; // 发送缓冲区
};