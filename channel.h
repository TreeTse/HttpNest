#ifndef CHANNEL_H
#define CHANNEL_H

#include <unistd.h>
#include <functional>
#include "epoller.h"
#include "socket.h"
#include "event_loop.h"
#include "connection.h"

class EventLoop;

class Channel
{
public:
    Channel(EventLoop *loop, int fd);
    ~Channel();
    int fd();
    void EnableET();
    uint32_t Events();
    uint32_t Revents();
    bool InEpoll();
    void SetInEpoll(bool inepoll);
    void SetRevents(uint32_t ev);
    void EnableReading();
    void DisableReading();
    void EnableWriting();
    void DisableWriting();
    void DisableAll();
    void Remove();
    void HandleEvent();
    void SetReadCallback(std::function<void()> cb);
    void SetCloseCallback(std::function<void()> cb);
    void SetErrorCallback(std::function<void()> cb);
    void SetWriteCallback(std::function<void()> cb);

private:
    int fd_ = -1;
    EventLoop *loop_ = nullptr;
    bool inEpoll_ = false;
    uint32_t events_ = 0;
    uint32_t revents_ = 0;
    std::function<void()> readCallback_;
    std::function<void()> closeCallback_;
    std::function<void()> errorCallback_;
    std::function<void()> writeCallback_;
};


#endif