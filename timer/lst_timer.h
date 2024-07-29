#ifndef LST_TIMER_H
#define LST_TIMER_H

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
#include <string>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <time.h>

#include "../log/log.h"

class util_timer;

struct client_data
{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

class util_timer
{
public:
    util_timer() : prev(NULL), next(NULL) {};
public:
    time_t expire;        // 任务的超时时间，这里使用绝对时间
    util_timer *prev;
    util_timer *next;
    void (*cb_func)(client_data *); // 任务回调函数
    client_data *user_data;         // 回调函数处理的客户数据，由定时器的执行者传递给回调函数
};

class sort_timer_lst
{
private:
    util_timer *head;
    util_timer *tail;
    void add_timer(util_timer *timer, util_timer *lst_head);

public:
    sort_timer_lst();
    ~sort_timer_lst();

    void add_timer(util_timer *timer);
    void adjust_timer(util_timer *timer);
    void del_timer(util_timer *timer);
    void tick();
};

//TODO: 工具类可以放到外面，待修改
class Utils
{
public:
    static int *u_pipefd;           // 用于统一事件源的管道，前端向u_pipefd[1]写，后端从u_pipefd[0]读
    sort_timer_lst m_timer_lst;     // 定时器容器
    static int u_epollfd;           // epoll文件描述符
    int m_TIMESLOT;                 // 最小超时单位

public:
    Utils(){}
    ~Utils(){}

    void init(int timeslot);

    // 对文件描述符设置非阻塞
    int setnonblocking(int fd);

    // 将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    // 信号处理函数
    static void sig_handler(int sig);

    // 设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    // 定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

};

void cb_func(client_data *user_data);

#endif