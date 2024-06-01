#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "../lock/locker.h"
#include "../mysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"

/*
 *  The http connection class is encapsulated by the master-slave state machine.
 *  Among them, the master state machine calls the slave state machine internally, 
 *  and the slave state machine transmits the processing state and data to the master state machine
 */

class HttpConn
{
public:
    static const int kFilenameLen = 200;
    static const int kReadBufferSize = 2048;
    static const int kWriteBufferSize = 1024;
    enum Method
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CheckState
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HttpCode
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        COUNTER_REQUEST,//req_counter
        CLOSED_CONNECTION
    };
    enum LineStatus
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    HttpConn() {}
    ~HttpConn() {}

public:
    void Init(int sockfd, const sockaddr_in &addr, char *root, int trig_mode,
                    int close_log, string user, string passwd, string sqlname);
    void CloseConn(bool real_close = true);
    void Process();
    bool ReadOnce();
    bool Write();
    sockaddr_in *GetAddress()
    {
        return &address_;
    }
    void InitMysqlResult(ConnectionPool *connPool);
    int timerFlag_;
    int improv_;

private:
    void Init();
    HttpCode ProcessRead();
    bool ProcessWrite(HttpCode ret);
    HttpCode ParseRequestLine(char *text);
    HttpCode ParseHeader(char *text);
    HttpCode ParseContent(char *text);
    HttpCode DoRequest();
    // startLine_: parsed characters; GetLine: set the pointer to the unprocessed character
    char *GetLine() { return readBuf_ + startLine_; };
    LineStatus ParseLine();
    void Unmap();
    bool AddResponse(const char *format, ...);
    bool AddContent(const char *content);
    bool AddStatusLine(int status, const char *title);
    bool AddHeaders(int content_length);
    bool AddContentType();
    bool AddContentLength(int content_length);
    bool AddLinger();
    bool AddBlankLine();

public:
    static int epollFd_;
    static int userCount_;
    static int userReqCount_;// Calculate the number of client req_count
    MYSQL *mysql_;
    int state_;  //read as 0, write as 1

private:
    bool isCounter_;
    int sockFd_;
    sockaddr_in address_;
    char readBuf_[kReadBufferSize];
    int readIdx_;
    int checkedIdx_;
    int startLine_;
    char writeBuf_[kWriteBufferSize];
    int writeIdx_;
    CheckState checkState_;
    Method method_;

    char realFile_[kFilenameLen];
    char *url_;
    char *version_;
    char *host_;
    int contentLength_;
    bool Linger_;

    char *fileAddress_;
    struct stat fileStat_;
    struct iovec iv_[2];
    int ivCount_;
    int isCgi_;
    char *string_;
    int bytesToSend_;
    int bytesHaveSend_;
    char *docRoot_;

    int trigMode_;
    int optCloseLog_;

    char sqlUser_[100];
    char sqlPasswd_[100];
    char sqlName_[100];
};

#endif
