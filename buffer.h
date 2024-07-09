#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <string>
#include <assert.h>
#include <cstring>
#include <unistd.h>
#include <sys/uio.h>
#include <errno.h>

class Buffer {
public:
    Buffer(int initBufferSize = 1024);
    ~Buffer() = default;

    size_t WritableBytes() const;
    size_t ReadableBytes() const;
    const char* Peek() const;
    void Retrieve(size_t len);
    void RetrieveUntil(const char* end);
    void RetrieveAll();
    void HasWritten(size_t len);
    ssize_t ReadFd(int fd, int* saveErrno);
    void Append(const std::string &str);
    void Append(const char* str, size_t len);
    char* BeginWrite();
    const char* BeginWriteConst() const;
    std::string RetrieveAllToStr();
    std::string RetrieveAsStr(size_t len);

private:
    const char* BeginPtr_() const;
    char* BeginPtr_();
    void EnsureWriteable(size_t len);
    void MakeSpace(size_t len);
    size_t PrependableBytes() const;

    std::vector<char> buffer_;
    size_t readPos_;//todo:thread safety
    size_t writePos_;
};

#endif