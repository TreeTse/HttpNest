#include "connection.h"

Connection::Connection(EventLoop *loop, std::unique_ptr<Socket> clientSock)
                :loop_(loop), clientSock_(std::move(clientSock)), disconnect_(false),
                 clientChannel_(new Channel(loop_, clientSock_->fd()))
{
    clientChannel_->SetReadCallback(std::bind(&Connection::OnMessage, this));
    clientChannel_->SetCloseCallback(std::bind(&Connection::CloseCallback, this));
    clientChannel_->SetErrorCallback(std::bind(&Connection::ErrorCallback, this));
    clientChannel_->SetWriteCallback(std::bind(&Connection::WriteCallback, this));
    clientChannel_->EnableET();
    clientChannel_->EnableReading();
}

Connection::~Connection()
{}

int Connection::fd() const
{
    return clientSock_->fd();
}

std::string Connection::ip() const
{
    return clientSock_->ip();
}

uint16_t Connection::port() const
{
    return clientSock_->port();
}

void Connection::CloseCallback()
{
    disconnect_ = true;
    clientChannel_->Remove();
    closeCallback_(shared_from_this());
}

void Connection::ErrorCallback()
{
    disconnect_ = true;
    clientChannel_->Remove();
    errorCallback_(shared_from_this());
}

void Connection::WriteCallback()
{
    int writen = send(fd(), outputBuffer_.Peek(), outputBuffer_.ReadableBytes(), 0);
    if (writen > 0) outputBuffer_.Retrieve(writen);
    if (outputBuffer_.ReadableBytes() == 0) {
        clientChannel_->DisableWriting();
        sendCompleteCallback_(shared_from_this());
    }
}

void Connection::SetCloseCallback(std::function<void(spConnection)> cb)
{
    closeCallback_ = cb;
}

void Connection::SetErrorCallback(std::function<void(spConnection)> cb)
{
    errorCallback_ = cb;
}

void Connection::SetOnMessageCallback(std::function<void(spConnection, Buffer &buf)> cb)
{
    onMessageCallback_ = cb;
}

void Connection::SetSendCompleteCallback(std::function<void(spConnection)> cb)
{
    sendCompleteCallback_ = cb;
}

void Connection::OnMessage()
{
    char buffer[1024];//todo:reduce copy
    Buffer inputBuffer;
    while (true) {
        bzero(&buffer, sizeof(buffer));
        ssize_t nread = read(fd(), buffer, sizeof(buffer));
        if (nread > 0) {
            inputBuffer.Append(buffer, nread);
        } else if (nread == -1 && errno == EINTR) {
            continue;
        } else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
            while (true) {
                if (inputBuffer.ReadableBytes() <= 0) break;

                lastTime_ = TimeStamp::Now();

                onMessageCallback_(shared_from_this(), inputBuffer);
            }
            break;
        } else if (nread == 0) {
            CloseCallback();
            break;
        }
    }
}

void Connection::Send(const char *data, size_t size)
{
    if (disconnect_ == true) {
        LOG_INFO("client disconnected");
        return;
    }
    if (loop_->IsInLoopThread()) {
        SendInLoop(data, size);
    } else {
        char *pp = new char[size + 1];
        strncpy(pp, data, size);
        pp[size] = '\0';
        loop_->QueueInLoop(std::bind(&Connection::SendInLoop, this, pp, size));//fixme
    }
}

void Connection::SendInLoop(const char *data, size_t size)
{
    outputBuffer_.Append(data, size);
    clientChannel_->EnableWriting();
}

bool Connection::IsTimeOut(time_t now, int timeout)
{
    return now - lastTime_.Toint() > timeout;
}