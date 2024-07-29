#include "lst_timer.h"
#include "../http/http_conn.h"

// 定时器容器构造函数
sort_timer_lst::sort_timer_lst()
{
    head = NULL;
    tail = NULL;
}

sort_timer_lst::~sort_timer_lst()
{
    util_timer *tmp = head;
    while(tmp)
    {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer)
{
    if (!timer)
        return ;

    if(!head)
    {
        head = tail = timer;
        return ;
    }

    if(timer->expire < head->expire)
    {
        timer->next = head;
        head->prev = timer;
        head = timer;
        return ;
    }
    add_timer(timer, head);
}

void sort_timer_lst::adjust_timer(util_timer *timer)
{
    if(!timer)
        return ;

    util_timer *tmp = timer->next;
    if(!tmp || (timer->expire < tmp->expire))
        return ;
    
    if (timer == head)
    {
        head = head->next;
        head->prev = NULL;
        timer->next = NULL;
        add_timer(timer, head);
    }
    else
    {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}

void sort_timer_lst::del_timer(util_timer *timer)
{
    if (!timer)
        return ;
    
    if (timer == head && timer == tail)
    {
        delete timer;
        head = NULL;
        tail = NULL;
        return ;
    }

    if(timer == head)
    {
        head = head->next;
        head->prev = NULL;
        delete timer;
        return ;
    }

    if (timer == tail)
    {
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return ;
    }

    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
    return ;    
}

// 定时任务处理函数
void sort_timer_lst::tick()
{
    if (!head)
        return ;

    time_t cur = time(NULL);
    util_timer *tmp = head;

    while (tmp)
    {
        if (cur < tmp->expire)
            break;
        
        // 调用回调函数处理任务
        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if (head)
        {
            head->prev = NULL;
        }
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer, util_timer *lst_head)
{
    util_timer *prev = lst_head;
    util_timer *tmp = prev->next;
    while (tmp)
    {
        if (timer->expire < tmp->expire)
        {
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    if (!tmp)
    {
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tail = timer;
    }
}

// Utils类的静态成员初始化
void Utils::init(int timeslot)
{
    m_TIMESLOT = timeslot;
}

// 对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    // 获取文件描述符的标志
    int old_option = fcntl(fd, F_GETFL);
    // 位或操作，设置文件描述符为非阻塞
    int new_option = old_option | O_NONBLOCK;
    // 将新的文件描述符设置为非阻塞
    fcntl(fd, F_SETFL, new_option);
    // 返回旧的文件描述符（为什么）
    return old_option;
}

// 向epoll内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
// epollfd: epoll文件描述符
// fd: 文件描述符
// one_shot: 是否开启EPOLLONESHOT
// TRIGMode: 事件触发模式
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event = {0};
    event.data.fd = fd;
    if (TRIGMode == 1)
    {
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    }
    else
    {
        event.events = EPOLLIN | EPOLLRDHUP;
    }
    if (one_shot)
    {
        event.events |= EPOLLONESHOT;
    }
    // 注册事件
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

// 信号处理函数
void Utils::sig_handler(int sig)
{
    // 保留原来的errno，在函数最后恢复，保证函数的可重入性
    int save_errno = errno;
    int msg = sig;
    // 将信号值从管道写端写入，传输字符类型，而不是整型
    send(u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

// 设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
    {
        sa.sa_flags |= SA_RESTART;
    }
    // 初始化信号集
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

// 定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    m_timer_lst.tick();
    // 一次alarm调用只会引起一次SIGALRM信号，所以需要重新定时，以不断触发SIGALRM信号
    alarm(m_TIMESLOT);
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

// 定义静态成员
int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count --;
}