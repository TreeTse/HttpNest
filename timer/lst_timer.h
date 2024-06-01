#ifndef LST_TIMER
#define LST_TIMER

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

#include <time.h>
#include "../log/log.h"

class UtilTimer;

struct ClientData
{
    sockaddr_in address;
    int sockfd;
    UtilTimer *timer;
};

class UtilTimer
{
public:
    UtilTimer() : prev_(NULL), next_(NULL) {}

public:
    time_t expire_; // connection time + fixed time (TIMESLOT)
    void (* cb_func_)(ClientData *);
    ClientData *userData_;
    UtilTimer *prev_;
    UtilTimer *next_;
};

class SortTimerLst
{
public:
    SortTimerLst();
    ~SortTimerLst();
    void AddTimer(UtilTimer *timer);
    void AdjustTimer(UtilTimer *timer);
    void DelTimer(UtilTimer *timer);
    void Tick();

private:
    void AddTimer(UtilTimer *timer, UtilTimer *lst_head);

    UtilTimer *head_;
    UtilTimer *tail_;
};

class Utils
{
public:
    Utils() {}
    ~Utils() {}
    void Init(int timeslot);
    int SetNonBlocking(int fd);
    void Addfd(int epollfd, int fd, bool one_shot, int trig_mode);
    static void SigHandler(int sig);
    void Addsig(int sig, void(handler)(int), bool restart = true);
    void TimerHandler();
    void ShowError(int connfd, const char *info);

public:
    static int *pipeFd_;
    SortTimerLst timerLst_;
    static int epollFd_;
    int timeslot_;
};

void TimerCbFunc(ClientData *user_data);

#endif
