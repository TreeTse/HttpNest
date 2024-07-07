#include "lst_timer.h"
#include "../http/http_conn.h"

SortTimerLst::SortTimerLst()
{
    head_ = NULL;
    tail_ = NULL;
}
SortTimerLst::~SortTimerLst()
{
    UtilTimer *tmp = head_;
    while (tmp) {
        head_ = tmp->next_;
        delete tmp;
        tmp = head_;
    }
}

void SortTimerLst::AddTimer(UtilTimer *timer)
{
    if (!timer) {
        return;
    }
    if (!head_) {
        head_ = tail_ = timer;
        return;
    }
    if (timer->expire_ < head_->expire_) {
        timer->next_ = head_;
        head_->prev_ = timer;
        head_ = timer;
        return;
    }
    AddTimer(timer, head_);
}

void SortTimerLst::AdjustTimer(UtilTimer *timer)
{
    if (!timer) {
        return;
    }
    UtilTimer *tmp = timer->next_;
    if (!tmp || (timer->expire_ < tmp->expire_)) {
        return;
    }
    if (timer == head_) {
        head_ = head_->next_;
        head_->prev_ = NULL;
        timer->next_ = NULL;
        AddTimer(timer, head_);
    } else {
        timer->prev_->next_ = timer->next_;
        timer->next_->prev_ = timer->prev_;
        AddTimer(timer, timer->next_);
    }
}

void SortTimerLst::DelTimer(UtilTimer *timer)
{
    if (!timer) {
        return;
    }
    // When there is only one timer in the linked list
    if ((timer == head_) && (timer == tail_)) {
        delete timer;
        head_ = NULL;
        tail_ = NULL;
        return;
    }
    // Deleted timer is the head_ node
    if (timer == head_) {
        head_ = head_->next_;
        head_->prev_ = NULL;
        delete timer;
        return;
    }
    // Deleted timer is the tail_ node
    if (timer == tail_) {
        tail_ = tail_->prev_;
        tail_->next_ = NULL;
        delete timer;
        return;
    }
    // Normal linked list deletion
    timer->prev_->next_ = timer->next_;
    timer->next_->prev_ = timer->prev_;
    delete timer;
}

// Handle expired timers in linked list
void SortTimerLst::Tick()
{
    if (!head_) {
        return;
    }
    
    time_t cur = time(NULL);
    UtilTimer *tmp = head_;
    while (tmp) {
        if (cur < tmp->expire_) {
            break;
        }
        tmp->cb_func_(tmp->userData_);
        head_ = tmp->next_;
        if (head_) {
            head_->prev_ = NULL;
        }
        delete tmp;
        tmp = head_;
    }
}

void SortTimerLst::AddTimer(UtilTimer *timer, UtilTimer *lst_head)
{
    UtilTimer *prev_ = lst_head;
    UtilTimer *tmp = prev_->next_;
    while (tmp) {
        if (timer->expire_ < tmp->expire_) {
            prev_->next_ = timer;
            timer->next_ = tmp;
            tmp->prev_ = timer;
            timer->prev_ = prev_;
            break;
        }
        prev_ = tmp;
        tmp = tmp->next_;
    }
    // The target timer needs to be placed at the tail_ node
    if (!tmp) {
        prev_->next_ = timer;
        timer->prev_ = prev_;
        timer->next_ = NULL;
        tail_ = timer;
    }
}

void Utils::Init(int timeslot)
{
    timeslot_ = timeslot;
}

/* Signal handling functions only send signals to notify the main loop, 
 * which shortens the asyn execution time and reduces the impact on the main program 
 */
void Utils::SigHandler(int sig)
{
    // To ensure the reentrancy of the function, keep the original errno
    int saveErrno = errno;
    int msg = sig;
    send(pipeFd_[1], (char *)&msg, 1, 0);
    errno = saveErrno;
}

// Set signal function
void Utils::Addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART; // Make system calls interrupted by signals automatically re-initiate
    sigfillset(&sa.sa_mask);       // Add all signals to signal set
    assert(sigaction(sig, &sa, NULL) != -1);  // Execute the sigaction function
}

void Utils::TimerHandler()
{
    timerLst_.Tick();
    alarm(timeslot_);
}

void Utils::ShowError(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::pipeFd_ = 0;
int Utils::epollFd_ = 0;

class Utils;

void TimerCbFunc(ClientData *user_data)
{
    epoll_ctl(Utils::epollFd_, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    HttpConn::userCount_--;
}
