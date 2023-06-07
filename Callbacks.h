#pragma once

#include <memory>
#include <functional>

class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&,
                                        Buffer*,
                                        Timestamp)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;
// 高水位，发送方发送快，接收方接收慢，会造成数据丢失