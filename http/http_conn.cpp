#include "http_conn.h"
#include <fstream>

const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

Locker gLock;
map<string, string> gUsers;

// Load the username and password in the database into the server's map
void HttpConn::InitMysqlResult(ConnectionPool *connPool)
{
    // Get a connection from the connection pool
    MYSQL *mysql = NULL;
    ConnectionRAII mySqlcon(&mysql, connPool);

    //retrieve account data from user table
    if (mysql_query(mysql, "SELECT username,passwd FROM user")) {
        LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
    }

    // Retrieve the complete result set from the table
    MYSQL_RES *result = mysql_store_result(mysql);

    // Returns the number of columns in the result set
    int numFields = mysql_num_fields(result);

    // Returns an array of all field structures
    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    while (MYSQL_ROW row = mysql_fetch_row(result)) {
        string temp1(row[0]);
        string temp2(row[1]);
        gUsers[temp1] = temp2;
    }
}

int SetNonBlocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void Addfd(int epollfd, int fd, bool one_shot, int trig_mode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == trig_mode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    SetNonBlocking(fd);
}

void RemoveFd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void ModFd(int epollfd, int fd, int ev, int trig_mode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == trig_mode)
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    else
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int HttpConn::userCount_ = 0;
int HttpConn::epollFd_ = -1;
int HttpConn::userReqCount_ = 0;

void HttpConn::CloseConn(bool real_close)
{
    if (real_close && (sockFd_ != -1)) {
        printf("HttpConn::CloseConn %d\n", sockFd_);
        RemoveFd(epollFd_, sockFd_);
        sockFd_ = -1;
        userCount_--;
    }
}

void HttpConn::Init(int sockfd, const sockaddr_in &addr, char *root, int trig_mode,
                    int close_log, string user, string passwd, string sqlname)
{
    sockFd_ = sockfd;
    address_ = addr;

    Addfd(epollFd_, sockfd, true, trigMode_);//////todo
    userCount_++;

    docRoot_ = root;
    trigMode_ = trig_mode;
    optCloseLog_ = close_log;

    strcpy(sqlUser_, user.c_str());
    strcpy(sqlPasswd_, passwd.c_str());
    strcpy(sqlName_, sqlname.c_str());

    Init();
}

void HttpConn::Init()
{
    mysql_ = NULL;
    bytesToSend_ = 0;
    bytesHaveSend_ = 0;
    checkState_ = CHECK_STATE_REQUESTLINE;
    Linger_ = true;
    method_ = GET;
    url_ = 0;
    version_ = 0;
    contentLength_ = 0;
    host_ = 0;
    startLine_ = 0;
    checkedIdx_ = 0;
    readIdx_ = 0;
    writeIdx_ = 0;
    isCgi_ = 0;
    state_ = 0;
    timerFlag_ = 0;
    improv_ = 0;
    isCounter_ = false; // req_counter

    memset(readBuf_, '\0', kReadBufferSize);
    memset(writeBuf_, '\0', kWriteBufferSize);
    memset(realFile_, '\0', kFilenameLen);
}

/*  Slave state machine, for analyze a line of content.
 *  checkedIdx_: points to the byte currently being analyzed
 *  readIdx_: points to the next byte from the end of the data in buffer readBuf_
 */
HttpConn::LineStatus HttpConn::ParseLine()
{
    char temp;
    for (; checkedIdx_ < readIdx_; ++checkedIdx_) {
        temp = readBuf_[checkedIdx_];
        if (temp == '\r') {
            /* When the next character reaches the end of the buffer, 
             * the reception is incomplete and needs to continue to receive
             */
            if ((checkedIdx_ + 1) == readIdx_) {
                return LINE_OPEN;
            }
            else if (readBuf_[checkedIdx_ + 1] == '\n') {
                readBuf_[checkedIdx_++] = '\0';
                readBuf_[checkedIdx_++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        } else if (temp == '\n') {
            // If the previous character is \r, complete
            if (checkedIdx_ > 1 && readBuf_[checkedIdx_ - 1] == '\r') {
                readBuf_[checkedIdx_ - 1] = '\0';
                readBuf_[checkedIdx_++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

/* Read client data in a loop until there is no data to read or 
 * the peer closes the connection
 */
bool HttpConn::ReadOnce()
{
    if (readIdx_ >= kReadBufferSize) {
        return false;
    }
    int bytesRead = 0;

    if (0 == trigMode_) { // LT
        bytesRead = recv(sockFd_, readBuf_ + readIdx_, kReadBufferSize - readIdx_, 0);
        readIdx_ += bytesRead;

        if (bytesRead <= 0) {
            return false;
        }

        return true;
    } else { // ET
        while (true) {
            bytesRead = recv(sockFd_, readBuf_ + readIdx_, kReadBufferSize - readIdx_, 0);
            if (bytesRead == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                return false;
            }
            else if (bytesRead == 0) {
                return false;
            }
            readIdx_ += bytesRead;
        }
        return true;
    }
}

// The slave state machine has changed the \r\n to \0\0
HttpConn::HttpCode HttpConn::ParseRequestLine(char *text)
{
    url_ = strpbrk(text, " \t");
    if (!url_) {
        return BAD_REQUEST;
    }
    *url_++ = '\0'; // For taking out the previous data
    char *method = text;
    /*if (strcasecmp(method, "GET") == 0) {
        method_ = GET;
    }*/
    if(method[0] == 'G') {
        method_ = GET;
    }
    else if (strcasecmp(method, "POST") == 0) {
        method_ = POST;
        isCgi_ = 1;
    }
    else
        return BAD_REQUEST;
    url_ += strspn(url_, " \t");
    version_ = strpbrk(url_, " \t");
    if (!version_)
        return BAD_REQUEST;
    *version_++ = '\0';
    version_ += strspn(version_, " \t");
    if (strcasecmp(version_, "HTTP/1.1") != 0) // Only support http1.1
        return BAD_REQUEST;
    /*if (strncasecmp(url_, "http://", 7) == 0) // Ignore for high performance
    {
        url_ += 7;
        url_ = strchr(url_, '/');
    }
    if (strncasecmp(url_, "https://", 8) == 0)
    {
        url_ += 8;
        url_ = strchr(url_, '/');
    }*/
    if (!url_ || url_[0] != '/')
        return BAD_REQUEST;
    if (strlen(url_) == 1)
        strcat(url_, "judge.html");
    if(!strcmp(url_, "/req_counter")) {
        isCounter_ = true;  //req_counter
    }
    checkState_ = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

HttpConn::HttpCode HttpConn::ParseHeader(char *text)
{
    if (text[0] == '\0') { // Determine whether it is a blank line or a request header
        if (contentLength_ != 0) { // Determine whether it is a GET or POST
            checkState_ = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    } else if (strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0) {
            Linger_ = true;
        }
    } else if (strncasecmp(text, "Content-length:", 15) == 0) {
        text += 15;
        text += strspn(text, " \t");
        contentLength_ = atol(text);
    } else if (strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " \t");
        host_ = text;
    } else {
        LOG_INFO("oop!unknow header: %s", text);
    }
    return NO_REQUEST;
}

HttpConn::HttpCode HttpConn::ParseContent(char *text)
{
    if (readIdx_ >= (contentLength_ + checkedIdx_)) { // Determine whether the message body has been read
        text[contentLength_] = '\0';
        string_ = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

// master-slave state machine
HttpConn::HttpCode HttpConn::ProcessRead()
{
    LineStatus lineStatus = LINE_OK;
    HttpCode ret = NO_REQUEST;
    char *text = 0;

    while ((checkState_ == CHECK_STATE_CONTENT && lineStatus == LINE_OK) || ((lineStatus = ParseLine()) == LINE_OK)) {
        text = GetLine();
        //startLine_: starting position in the buffer of the line of data
        //checkedIdx_: the position read by the slave state machine in readBuf_
        startLine_ = checkedIdx_;
        LOG_INFO("%s", text);
        printf("read: %s\n", text);
        switch (checkState_) {
            case CHECK_STATE_REQUESTLINE: {
                ret = ParseRequestLine(text);
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            }
            case CHECK_STATE_HEADER: {
                ret = ParseHeader(text);
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                else if (ret == GET_REQUEST) { // After fully parsing the GET request, do response
                    return DoRequest();
                }
                break;
            }
            case CHECK_STATE_CONTENT: {
                ret = ParseContent(text);
                if (ret == GET_REQUEST)
                    return DoRequest();
                lineStatus = LINE_OPEN;
                break;
            }
            default:
                return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

HttpConn::HttpCode HttpConn::DoRequest()
{
    strcpy(realFile_, docRoot_);
    int len = strlen(docRoot_);
    const char *p = strrchr(url_, '/');

    // Login and registration verification
    if (isCgi_ == 1 && (*(p + 1) == '2' || *(p + 1) == '3')) {
        char flag = url_[1];

        char *urlReal = (char *)malloc(sizeof(char) * 200);
        strcpy(urlReal, "/");
        strcat(urlReal, url_ + 2);
        strncpy(realFile_ + len, urlReal, kFilenameLen - len - 1);
        free(urlReal);

        // eg:user=123&passwd=123
        char name[100], password[100];
        int i;
        for (i = 5; string_[i] != '&'; ++i)
            name[i - 5] = string_[i];
        name[i - 5] = '\0';

        int j = 0;
        for (i = i + 10; string_[i] != '\0'; ++i, ++j)
            password[j] = string_[i];
        password[j] = '\0';

        if (*(p + 1) == '3') {
            // If it is registered, check whether there is a duplicate name in the database
            char *sqlInsert = (char *)malloc(sizeof(char) * 200);
            strcpy(sqlInsert, "INSERT INTO user(username, passwd) VALUES(");
            strcat(sqlInsert, "'");
            strcat(sqlInsert, name);
            strcat(sqlInsert, "', '");
            strcat(sqlInsert, password);
            strcat(sqlInsert, "')");

            if (gUsers.find(name) == gUsers.end()) {
                gLock.Lock();
                int res = mysql_query(mysql_, sqlInsert);
                gUsers.insert(pair<string, string>(name, password));
                gLock.Unlock();

                if (!res)
                    strcpy(url_, "/log.html");
                else
                    strcpy(url_, "/registerError.html");
            } else {
                strcpy(url_, "/registerError.html");
            }
        }
        // If it is a login
        else if (*(p + 1) == '2') {
            if (gUsers.find(name) != gUsers.end() && gUsers[name] == password)
                strcpy(url_, "/welcome.html");
            else
                strcpy(url_, "/logError.html");
        }
    }

    if (*(p + 1) == '0') { // Jump to the registration interface
        char *realUrl = (char *)malloc(sizeof(char) * 200);
        strcpy(realUrl, "/register.html");
        strncpy(realFile_ + len, realUrl, strlen(realUrl));
        free(realUrl);
    } else if (*(p + 1) == '1') { // Jump to the login interface
        char *realUrl = (char *)malloc(sizeof(char) * 200);
        strcpy(realUrl, "/log.html");
        strncpy(realFile_ + len, realUrl, strlen(realUrl));
        free(realUrl);
    } else if (*(p + 1) == '5') {
        char *realUrl = (char *)malloc(sizeof(char) * 200);
        strcpy(realUrl, "/picture.html");
        strncpy(realFile_ + len, realUrl, strlen(realUrl));
        free(realUrl);
    } else if (*(p + 1) == '6') {
        char *realUrl = (char *)malloc(sizeof(char) * 200);
        strcpy(realUrl, "/video.html");
        strncpy(realFile_ + len, realUrl, strlen(realUrl));
        free(realUrl);
    } else if (isCounter_) { //req_counter
        __sync_add_and_fetch(&userReqCount_, 1);
        strncpy(realFile_ + len, url_, kFilenameLen - len - 1);
        return COUNTER_REQUEST;
    } else
        strncpy(realFile_ + len, url_, kFilenameLen - len - 1);

    if (stat(realFile_, &fileStat_) < 0)
        return NO_RESOURCE;

    // Whether it is readable
    if (!(fileStat_.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;

    // Whether it is dir
    if (S_ISDIR(fileStat_.st_mode))
        return BAD_REQUEST;

    int fd = open(realFile_, O_RDONLY);
    fileAddress_ = (char *)mmap(0, fileStat_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;
}

void HttpConn::Unmap()
{
    if (fileAddress_) {
        munmap(fileAddress_, fileStat_.st_size);
        fileAddress_ = 0;
    }
}

bool HttpConn::Write()
{
    int temp = 0;
    if (bytesToSend_ == 0) {
        ModFd(epollFd_, sockFd_, EPOLLIN, trigMode_);
        Init();
        return true;
    }

    while (1) {
        // Send response message
        temp = writev(sockFd_, iv_, ivCount_);
        if (temp < 0) {
            if (errno == EAGAIN) {
                /* Buffer full;When the write buffer changes from unwritable to writable, epollout is triggered
                 * the next request from the same user cannot be received immediately during this period, but 
                 * the integrity of the connection is guaranteed.
                 */
                printf("EAGAIN; write buffer full...\n");
                ModFd(epollFd_, sockFd_, EPOLLOUT, trigMode_);
                return true;
            }
            Unmap();
            return false;
        }

        bytesHaveSend_ += temp; // Update sent bytes
        bytesToSend_ -= temp;
        // The data of the first iovec header information has been sent
        if (bytesHaveSend_ >= iv_[0].iov_len) {
            iv_[0].iov_len = 0;
            iv_[1].iov_base = fileAddress_ + (bytesHaveSend_ - writeIdx_);
            iv_[1].iov_len = bytesToSend_;
        } else {
            iv_[0].iov_base = writeBuf_ + bytesHaveSend_;
            iv_[0].iov_len = iv_[0].iov_len - bytesHaveSend_;
        }

        if (bytesToSend_ <= 0) {// All data has been sent
            Unmap();
            ModFd(epollFd_, sockFd_, EPOLLIN, trigMode_); // Reset EPOLLONESHOT event
            if (Linger_) {
                Init();// Reset http, register the read event
                return true;
            }
            else {
                return false;
            }
        }
    }
}

bool HttpConn::AddResponse(const char *format, ...)
{
    if (writeIdx_ >= kWriteBufferSize)
        return false;
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(writeBuf_ + writeIdx_, kWriteBufferSize - 1 - writeIdx_, format, arg_list);
    // If the length of the written data exceeds the remaining space in the buffer, report error 
    if (len >= (kWriteBufferSize - 1 - writeIdx_)) {
        va_end(arg_list);
        return false;
    }
    writeIdx_ += len;
    va_end(arg_list);

    LOG_INFO("request:%s", writeBuf_);

    return true;
}

bool HttpConn::AddStatusLine(int status, const char *title)
{
    return AddResponse("%s %d %s\r\n", "HTTP/1.1", status, title);
}
// Text length, connection status, and blank lines
bool HttpConn::AddHeaders(int content_len)
{
    return AddContentLength(content_len) && AddLinger() &&
           AddBlankLine();
}
bool HttpConn::AddContentLength(int content_len)
{
    return AddResponse("Content-Length:%d\r\n", content_len);
}
bool HttpConn::AddContentType()
{
    return AddResponse("Content-Type:%s\r\n", "text/html");
}
bool HttpConn::AddLinger()
{
    return AddResponse("Connection:%s\r\n", (Linger_ == true) ? "keep-alive" : "close");
}
bool HttpConn::AddBlankLine()
{
    return AddResponse("%s", "\r\n");
}
bool HttpConn::AddContent(const char *content)
{
    return AddResponse("%s", content);
}

/* 1.request file exists: two iovec, one pointer to writeBuf_, other pointer to fileAddress_
 * 2.request error: only one iovec, pointer to writeBuf_
 */
bool HttpConn::ProcessWrite(HttpCode ret)
{
    switch (ret) {
        case COUNTER_REQUEST: {
            AddStatusLine(200, ok_200_title);
            char count[20];
            sprintf(count, "%d\n", userReqCount_);
            //printf("clinet request count:%d\n",userReqCount_);
            AddHeaders(strlen(count));
            if(!AddResponse("%s", count))
                return false;
            break;
        }
        case INTERNAL_ERROR: {
            AddStatusLine(500, error_500_title);
            AddHeaders(strlen(error_500_form));
            if (!AddContent(error_500_form))
                return false;
            break;
        }
        case BAD_REQUEST: {
            AddStatusLine(404, error_404_title);
            AddHeaders(strlen(error_404_form));
            if (!AddContent(error_404_form))
                return false;
            break;
        }
        case FORBIDDEN_REQUEST: {
            AddStatusLine(403, error_403_title);
            AddHeaders(strlen(error_403_form));
            if (!AddContent(error_403_form))
                return false;
            break;
        }
        case FILE_REQUEST: {
            AddStatusLine(200, ok_200_title);
            if (fileStat_.st_size != 0) {
                AddHeaders(fileStat_.st_size);
                iv_[0].iov_base = writeBuf_;
                iv_[0].iov_len = writeIdx_;
                iv_[1].iov_base = fileAddress_;
                iv_[1].iov_len = fileStat_.st_size;
                ivCount_ = 2;
                // Response header info + file size
                bytesToSend_ = writeIdx_ + fileStat_.st_size;
                return true;
            } else { // Blank html file
                const char *okString = "<html><body></body></html>";
                AddHeaders(strlen(okString));
                if (!AddContent(okString))
                    return false;
            }
        }
        default:
            return false;
    }
    iv_[0].iov_base = writeBuf_;
    iv_[0].iov_len = writeIdx_;
    ivCount_ = 1;
    bytesToSend_ = writeIdx_;
    return true;
}

void HttpConn::Process()
{
    HttpCode readRet = ProcessRead();
    // The request data is incomplete and needs to receiving more data
    if (readRet == NO_REQUEST) {
        ModFd(epollFd_, sockFd_, EPOLLIN, trigMode_);
        return;
    }
    bool writeRet = ProcessWrite(readRet);
    if (!writeRet) {
        CloseConn();
    }
    ModFd(epollFd_, sockFd_, EPOLLOUT, trigMode_);
}
