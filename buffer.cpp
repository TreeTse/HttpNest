#include "buffer.h"

Buffer::Buffer(int initBufferSize): buffer_(initBufferSize), readPos_(0), writePos_(0)
{}

size_t Buffer::WritableBytes() const
{
    return buffer_.size() - writePos_;
}

size_t Buffer::ReadableBytes() const
{
    return writePos_ - readPos_;
}

size_t Buffer::PrependableBytes() const
{
    return readPos_;
}

const char* Buffer::Peek() const
{
    return BeginPtr_() + readPos_;
}

char* Buffer::BeginPtr_()
{
    return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const
{
    return &*buffer_.begin();
}

void Buffer::Retrieve(size_t len)
{
    assert(len <= ReadableBytes());
    readPos_ += len;
}

void Buffer::RetrieveUntil(const char* end)
{
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

void Buffer::RetrieveAll()
{
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

void Buffer::HasWritten(size_t len)
{
    writePos_ += len;
}

char* Buffer::BeginWrite()
{
    return BeginPtr_() + writePos_;
}

const char* Buffer::BeginWriteConst() const
{
    return BeginPtr_() + writePos_;
}

void Buffer::Append(const char* str, size_t len)
{
    assert(str);
    EnsureWriteable(len);
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}

void Buffer::Append(const std::string &str)
{
    Append(str.data(), str.length());
}

void Buffer::EnsureWriteable(size_t len)
{
    if(WritableBytes() < len) {
        MakeSpace(len);
    }
    assert(WritableBytes() >= len);
}

ssize_t Buffer::ReadFd(int fd, int* saveErrno)
{
    char buff[65535];
    struct iovec iov[2];
    const size_t writable = WritableBytes();

    iov[0].iov_base = BeginPtr_() + writePos_;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *saveErrno = errno;
    } else if (static_cast<size_t>(len) <= writable) {
        writePos_ += len;
    } else {
        writePos_ = buffer_.size();
        Append(buff, len - writable);
    }
    return len;
}

std::string Buffer::RetrieveAsStr(size_t len)
{
    assert(len <= ReadableBytes());
    std::string str(Peek(), len);
    Retrieve(len);

    return str;
}

std::string Buffer::RetrieveAllToStr()
{
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();

    return str;
}

void Buffer::MakeSpace(size_t len)
{
    if (WritableBytes() + PrependableBytes() < len) {
        buffer_.resize(writePos_ + len + 1);
    } else { //read space can be utilized
        size_t readable = ReadableBytes();
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes());
    }
}