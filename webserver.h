#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include <memory>

#include "./threadpool/threadpool.h"
#include "./http/http_conn.h"
#include "epoller.h"

const int MAX_FD = 65536;           //max file descriptors
const int MAX_EVENT_NUMBER = 10000; //maximum number of events
const int TIMESLOT = 5;             //minimum timeout unit

class WebServer
{
public:
    WebServer();
    ~WebServer();
    void Init(int port, string user_name, string password, string database_name, 
              int async_write, int opt_linger, int trigmode, int sql_num, 
              int thread_num, int close_log, int actor_model);       
    void InitThreadPool();
    void InitSqlPool();
    void InitLogger();
    void TrigMode();
    void EventListen();
    void EventLoop();
    void Timer(int connfd, struct sockaddr_in client_address);
    void AdjustTimer(UtilTimer *timer);
    void DealTimer(UtilTimer *timer, int sockfd);
    bool DealClinetData();
    bool DealWithSignal(bool& timeout, bool& stop_server);
    void DealWithRead(int sockfd);
    void DealWithWrite(int sockfd);
    static int SetFdNonBlocking(int fd);

public:
    int listenFd_;
    int port_;
    int pipeFd_[2];
    char *root_;
    int asyncWrite_;
    int optCloseLog_;
    int actorModel_;

    HttpConn *users_;

    ConnectionPool *connPool_;
    string userName_;
    string passWord_;
    string databaseName_;
    int sqlNum_;

    ThreadPool<HttpConn> *threadPool_;
    int threadNum_;

    epoll_event events_[MAX_EVENT_NUMBER];

    int optLinger_;
    int optTrigMode_;
    int optListenTrigMode_;
    int optConnTrigMode_;

    ClientData *usersTimer_;
    Utils utils_;

    std::unique_ptr<Epoller> epoller_;
};
#endif
