#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <memory>
#include <map>
#include "event_loop.h"
#include "acceptor.h"
#include "threadpool.h"

class TcpServer
{
public:
    TcpServer(const std::string &ip, const uint16_t port, int threadNum);
    ~TcpServer();
    void Start();
    void Stop();
    void EpollTimeout(EventLoop *loop);
    void NewConnection(std::unique_ptr<Socket> clientSock);
    void CloseConnection(spConnection conn);
    void ErrorConnection(spConnection conn);
    void OnMessage(spConnection conn, Buffer &buf);
    void SendComplete(spConnection conn);
    void RemoveConnection(int fd);

    void SetCloseConnectionCb(std::function<void(spConnection)> fun);
    void SetErrorConnectionCb(std::function<void(spConnection)> fun);
    void SetOnMessageCb(std::function<void(spConnection, Buffer &buf)> fun);
    void SetNewConnectionCb(std::function<void(spConnection)> fun);
    void SetSendCompleteCb(std::function<void(spConnection)> fun);
    void SetTimeoutCb(std::function<void(EventLoop*)> fun);
    void SetRemoveConnectionCb(std::function<void(int)> fun);

private:
    int threadNum_;
    std::unique_ptr<EventLoop> mainLoop_;
    Acceptor acceptor_;
    ThreadPool threadpool_;
    std::vector<std::unique_ptr<EventLoop>> subLoops_;
    std::map<int, spConnection> conns_;
    std::mutex connMtx_;
    std::function<void(EventLoop*)> timeoutCallback_;
    std::function<void(spConnection)> closeConnectionCallback_;
    std::function<void(spConnection)> errorConnectionCallback_;
    std::function<void(spConnection conn, Buffer &buf)> onMessageCallback_;
    std::function<void(spConnection)> newConnectionCallback_;
    std::function<void(spConnection)> sendCompleteCallback_;
    std::function<void(int)> removeConnectionCallback_;
};


#endif