#include "webserver.h"

WebServer::WebServer()
{
    users_ = new HttpConn[MAX_FD];

    char serverPath[200];
    getcwd(serverPath, 200); // Current working directory
    char root[6] = "/root";
    root_ = (char *)malloc(strlen(serverPath) + strlen(root) + 1);
    strcpy(root_, serverPath);
    strcat(root_, root);
    printf("root_: %s\n", root_);

    usersTimer_ = new ClientData[MAX_FD];
}

WebServer::~WebServer()
{
    close(epollFd_);
    close(listenFd_);
    close(pipeFd_[1]);
    close(pipeFd_[0]);
    delete[] users_;
    delete[] usersTimer_;
    delete threadPool_;
}

void WebServer::Init(int port, string user_name, string password, string database_name, 
        int async_write, int opt_linger, int trigmode, int sql_num, int thread_num, 
        int close_log, int actor_model)
{
    port_ = port;
    userName_ = user_name;
    passWord_ = password;
    databaseName_ = database_name;
    sqlNum_ = sql_num;
    threadNum_ = thread_num;
    asyncWrite_ = async_write;
    optLinger_ = opt_linger;
    optTrigMode_ = trigmode;
    optCloseLog_ = close_log;
    actorModel_ = actor_model;
}

void WebServer::TrigMode()
{
    if (0 == optTrigMode_) { // LT + LT
        optListenTrigMode_ = 0;
        optConnTrigMode_ = 0;
    } else if (1 == optTrigMode_) { // LT + ET
        optListenTrigMode_ = 0;
        optConnTrigMode_ = 1;
    } else if (2 == optTrigMode_) { // ET + LT
        optListenTrigMode_ = 1;
        optConnTrigMode_ = 0;
    } else if (3 == optTrigMode_) { // ET + ET
        optListenTrigMode_ = 1;
        optConnTrigMode_ = 1;
    }
}

void WebServer::InitLogger()
{
    if (0 == optCloseLog_) {
        if (1 == asyncWrite_)
            Log::GetInstance()->Init("./ServerLog", optCloseLog_, 2000, 800000, 800);
        else
            Log::GetInstance()->Init("./ServerLog", optCloseLog_, 2000, 800000, 0);
    }
}

void WebServer::InitSqlPool()
{
    connPool_ = ConnectionPool::GetInstance();
    connPool_->Init("localhost", userName_, passWord_, databaseName_, 3306, sqlNum_, optCloseLog_);

    // Init database read table
    users_->InitMysqlResult(connPool_);
}

void WebServer::InitThreadPool()
{
    threadPool_ = new ThreadPool<HttpConn>(actorModel_, connPool_, threadNum_);
}

void WebServer::EventListen()
{
    listenFd_ = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenFd_ >= 0);

    if (0 == optLinger_) {
        struct linger tmp = {0, 1};
        setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    } else if (1 == optLinger_) {
        struct linger tmp = {1, 1};
        setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port_);
    int ret = 0;
    int reuse = 1;
    // SO_REUSEADDR: allow ports to be reused
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    ret = bind(listenFd_, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);

    ret = listen(listenFd_, 5);
    assert(ret >= 0);

    utils_.Init(TIMESLOT);

    epoll_event events[MAX_EVENT_NUMBER];
    epollFd_ = epoll_create(5);
    assert(epollFd_ != -1);

    utils_.Addfd(epollFd_, listenFd_, false, optListenTrigMode_);
    HttpConn::epollFd_ = epollFd_;

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipeFd_);
    assert(ret != -1);

    /* Send data to the socket buffer. 
     * If the buffer is full, it will block.
     * At this time, the execution time of signal processing will be further increased. 
     * For this reason, it is modified to non-blocking.
     */
    utils_.SetNonBlocking(pipeFd_[1]);
    utils_.Addfd(epollFd_, pipeFd_[0], false, 0);

    utils_.Addsig(SIGPIPE, SIG_IGN);
    utils_.Addsig(SIGALRM, utils_.SigHandler, false);
    utils_.Addsig(SIGTERM, utils_.SigHandler, false);

    alarm(TIMESLOT);

    Utils::pipeFd_ = pipeFd_;
    Utils::epollFd_ = epollFd_;
}

void WebServer::Timer(int connfd, struct sockaddr_in client_address)
{
    users_[connfd].Init(connfd, client_address, root_, optConnTrigMode_, optCloseLog_, userName_, passWord_, databaseName_);

    usersTimer_[connfd].address = client_address;
    usersTimer_[connfd].sockfd = connfd;
    UtilTimer *timer = new UtilTimer;
    timer->userData_ = &usersTimer_[connfd];
    timer->cb_func_ = TimerCbFunc;
    time_t cur = time(NULL);
    timer->expire_ = cur + 3 * TIMESLOT;
    usersTimer_[connfd].timer = timer;
    utils_.timerLst_.AddTimer(timer);
}

void WebServer::AdjustTimer(UtilTimer *timer)
{
    time_t cur = time(NULL);
    timer->expire_ = cur + 3 * TIMESLOT;
    utils_.timerLst_.AdjustTimer(timer);

    LOG_INFO("%s", "adjust timer once");
}

void WebServer::DealTimer(UtilTimer *timer, int sockfd)
{
    timer->cb_func_(&usersTimer_[sockfd]);
    if (timer) {
        utils_.timerLst_.DelTimer(timer);
    }

    LOG_INFO("close fd %d", usersTimer_[sockfd].sockfd);
}

bool WebServer::DealClinetData()
{
    struct sockaddr_in clientAddress;
    socklen_t clientAddrLength = sizeof(clientAddress);
    if (0 == optListenTrigMode_) { // LT
        int connfd = accept(listenFd_, (struct sockaddr *)&clientAddress, &clientAddrLength);
        if (connfd < 0) {
            LOG_ERROR("%s,errno is:%d", "accept error", errno);
            return false;
        }
        if (HttpConn::userCount_ >= MAX_FD) {
            utils_.ShowError(connfd, "Internal server busy");
            printf("Internal server busy\n");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        Timer(connfd, clientAddress);
    } else { // ET
        while (1) {
            int connfd = accept(listenFd_, (struct sockaddr *)&clientAddress, &clientAddrLength);
            if (connfd < 0) {
                LOG_ERROR("%s,errno is:%d", "accept error", errno);
                break;
            }
            if (HttpConn::userCount_ >= MAX_FD) {
                utils_.ShowError(connfd, "Internal server busy");
                printf("Internal server busy\n");
                LOG_ERROR("%s", "Internal server busy");
                break;
            }
            Timer(connfd, clientAddress);
        }
        return false;
    }
    return true;
}

bool WebServer::DealWithSignal(bool &timeout, bool &stop_server)
{
    int ret = 0;
    int sig;
    char signals[1024];
    ret = recv(pipeFd_[0], signals, sizeof(signals), 0);
    if (ret == -1) {
        return false;
    } else if (ret == 0) {
        return false;
    } else {
        for (int i = 0; i < ret; ++i) {
            switch (signals[i]) {
                case SIGALRM: {
                    timeout = true;
                    break;
                }
                case SIGTERM: {
                    stop_server = true;
                    break;
                }
            }
        }
    }
    return true;
}

void WebServer::DealWithRead(int sockfd)
{
    UtilTimer *timer = usersTimer_[sockfd].timer;

    if(0 == actorModel_) { // Proactor
        if (users_[sockfd].ReadOnce()) {
            LOG_INFO("deal with the client(%s)", inet_ntoa(users_[sockfd].GetAddress()->sin_addr));

            threadPool_->Append(users_ + sockfd);

            if (timer) {
                AdjustTimer(timer);
            }
        } else {
            DealTimer(timer, sockfd);
        }
    } else {
        if (timer) {
            AdjustTimer(timer);
        }

        threadPool_->Append(users_ + sockfd, 0);

        while (true) {
            if (1 == users_[sockfd].improv_) { // fixme: don't let the main thread block here
                if (1 == users_[sockfd].timerFlag_) {
                    DealTimer(timer, sockfd);
                    users_[sockfd].timerFlag_ = 0;
                }
                users_[sockfd].improv_ = 0;
                break;
            }
        }
    }
}

void WebServer::DealWithWrite(int sockfd)
{
    UtilTimer *timer = usersTimer_[sockfd].timer;
    if (1 == actorModel_) { // Reactor
        if (timer) {
            AdjustTimer(timer);
        }

        threadPool_->Append(users_ + sockfd, 1);

        while (true) {
            if (1 == users_[sockfd].improv_) {
                if (1 == users_[sockfd].timerFlag_) {
                    DealTimer(timer, sockfd);
                    users_[sockfd].timerFlag_ = 0;
                }
                users_[sockfd].improv_ = 0;
                break;
            }
        }
    } else { // Proactor
        if (users_[sockfd].Write()) {
            LOG_INFO("send data to the client(%s)", inet_ntoa(users_[sockfd].GetAddress()->sin_addr));

            if (timer) {
                AdjustTimer(timer);
            }
        } else {
            DealTimer(timer, sockfd);
        }
    }
}

void WebServer::EventLoop()
{
    bool timeout = false;
    bool stopServer = false;

    while (!stopServer) {
        int number = epoll_wait(epollFd_, events_, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        /* Handle ready events */
        for (int i = 0; i < number; i++) {
            int sockfd = events_[i].data.fd;

            // Listen for new user connections
            if (sockfd == listenFd_) {
                bool flag = DealClinetData();
                if (false == flag)
                    continue;
            }
            // Exception, close the client connection and delete the user's timer
            else if (events_[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                UtilTimer *timer = usersTimer_[sockfd].timer;
                DealTimer(timer, sockfd);
            }
            // Handle signal
            else if ((sockfd == pipeFd_[0]) && (events_[i].events & EPOLLIN)) {
                bool flag = DealWithSignal(timeout, stopServer);
                if (false == flag)
                    LOG_ERROR("%s", "DealWithSignal failure");
            }
            // Process data received on the client connection
            else if (events_[i].events & EPOLLIN) {
                DealWithRead(sockfd);
            }
            else if (events_[i].events & EPOLLOUT) {
                DealWithWrite(sockfd);
            }
        }
        // After the read and write events are completed, the timer is processed
        if (timeout) {
            utils_.TimerHandler();
            LOG_INFO("%s", "timer tick");
            timeout = false;
        }
    }
}