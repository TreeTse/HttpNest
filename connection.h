#ifndef CONNECTION_H
#define CONNECTION_H

#include <memory>
#include <atomic>
#include "socket.h"
#include "channel.h"
#include "event_loop.h"
#include "buffer.h"
#include "timestamp.h"

class Channel;
class EventLoop;

class Connection;
using spConnection = std::shared_ptr<Connection>;

class Connection: public std::enable_shared_from_this<Connection>
{
public:
    Connection(EventLoop *loop, std::unique_ptr<Socket> clientSock);
    ~Connection();
    int fd() const;
    std::string ip() const;
    uint16_t port() const;
    void CloseCallback();
    void ErrorCallback();
    void WriteCallback();
    void OnMessage();
    void SetCloseCallback(std::function<void(spConnection)> cb);
    void SetErrorCallback(std::function<void(spConnection)> cb);
    void SetOnMessageCallback(std::function<void(spConnection, Buffer &buf)> cb);
    void SetSendCompleteCallback(std::function<void(spConnection)> cb);
    void Send(const char *data, size_t size);
    void SendInLoop(const char *data, size_t size);
    bool IsTimeOut(time_t now, int timeout);

private:
    EventLoop *loop_;
    std::unique_ptr<Socket> clientSock_;
    std::unique_ptr<Channel> clientChannel_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;
    std::atomic_bool disconnect_;
    std::function<void(spConnection)> closeCallback_;
    std::function<void(spConnection)> errorCallback_;
    std::function<void(spConnection, Buffer &buf)> onMessageCallback_;
    std::function<void(spConnection)> sendCompleteCallback_;
    TimeStamp lastTime_;
};


#endif