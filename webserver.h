#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <cassert>
#include <sys/epoll.h>

#include "threadpool/threadpool.h"
#include "http/http_conn.h"
#include "timer/lst_timer.h"

#include <iostream>
#include <string>
using namespace std;

const int MAX_FD = 65536;           // 最大文件描述符
const int MAX_EVENT_NUMBER = 10000; // 最大事件数
const int TIMESLOT = 5;             // 超时时间

class WebServer 
{
public:
    WebServer();
    ~WebServer();

    void init(int port, string user, string password, string database,
              int log_write, int opt_linger, int trig_mode, int sql_num,
              int thread_num, int close_log, int actor_model);
    void thread_pool();
    void sql_pool();
    void log_write();
    void trig_mode();
    void eventListen();
    void eventLoop();
    void timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(util_timer *timer);
    void deal_timer(util_timer *timer, int sockfd);
    bool dealclientdata();
    bool dealwithsignal(bool& timeout, bool& stop_server);
    void dealwithread(int sockfd);
    void dealwithwrite(int sockfd);

public:
    // 基础
    int m_port;                     // 端口号
    char *m_root;                   // 资源目录
    int m_log_write;                // 日志写入方式
    int m_close_log;                // 关闭日志
    int m_actor_model;              // 并发模型选择

    int m_pipefd[2];                // 管道
    int m_epollfd;                  // epoll
    http_conn *users;               // 用户连接数组

    // 数据库相关
    connection_pool *m_connPool;    // 数据库连接池
    string m_user;                  // 登录数据库用户名
    string m_password;              // 登录数据库密码
    string m_databaseName;          // 使用数据库名
    int m_sql_num;                  // 数据库连接池数量

    // 线程池相关
    threadpool<http_conn> *m_pool;  // 线程池
    int m_thread_num;               // 线程池数量

    // epoll_event相关
    epoll_event events[MAX_EVENT_NUMBER]; // epoll_event数组

    int m_listenfd;                 // 监听套接字
    int m_OPT_LINGER;               // 优雅关闭连接
    int m_TRIGMode;                 // 触发模式
    int m_LISTENTrigmode;           // listenfd触发模式
    int m_CONNTrigmode;             // connfd触发模式

    // 定时器相关
    client_data *users_timer;       // 用户数据数组
    Utils utils;
};

#endif