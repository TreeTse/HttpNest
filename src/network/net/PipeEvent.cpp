#include "PipeEvent.h"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <errno.h>
#include "base/LogStream.h"

using namespace nest::net;

PipeEvent::PipeEvent(EventLoop *loop)
: Channel(loop)
{
    int fd[2] = {0,};
    auto ret = ::pipe2(fd, O_NONBLOCK);
    if(ret < 0) {
        LOG_ERROR << "pipe open failed.";
        exit(-1);
    }
    fd_ = fd[0];
    writefd_ = fd[1];
}

PipeEvent::~PipeEvent()
{
    if(writefd_>0) {
        ::close(writefd_);
        writefd_ = -1;
    }
}

void PipeEvent::OnRead()
{
    int64_t tmp = 0;
    auto ret = ::read(fd_, &tmp, sizeof(tmp));
    if(ret < 0) {
        LOG_ERROR << "pipe read error, error:" << errno;
        return;
    }
    //std::cout << " pipe read tmp:" << tmp << std::endl;
}

void PipeEvent::OnClose()
{
    if(writefd_ > 0) {
        ::close(writefd_);
        writefd_ = -1;
    }
}

void PipeEvent::OnError(const std::string &msg)
{
    std::cout << "error:" << msg << std::endl;
}

void PipeEvent::Write(const char *data, size_t len)
{
    ::write(writefd_, data, len);
}