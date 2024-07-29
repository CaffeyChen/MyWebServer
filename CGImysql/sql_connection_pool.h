#ifndef _CONNECTION_POOL_H
#define _CONNECTION_POOL_H

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string>
#include <string.h>
#include <iostream>

#include "../lock/locker.h"
#include "../log/log.h"

using namespace std;

class connection_pool
{
private:
    connection_pool();
    ~connection_pool();

    int m_MaxConn;         // 最大连接数
    int m_CurConn;         // 当前已使用的连接数
    int m_FreeConn;        // 当前空闲的连接数
    locker lock;            // 互斥锁
    list<MYSQL *> connList; // 连接池
    sem reserver;           // 保持连接池中连接的信号量

// TODO: 以后可以调整私有公有成员的位置
public:
    string m_url;           // 主机地址
    string m_Port;          // 数据库端口号
    string m_User;          // 登陆数据库用户名
    string m_PassWord;      // 登陆数据库密码
    string m_DatabaseName;  // 使用数据库名
    int m_close_log;        // 日志开关

    MYSQL *GetConnection(); // 获取数据库连接
    bool ReleaseConnection(MYSQL *conn); // 释放连接
    int GetFreeConn();      // 获取所有空闲连接
    void DestroyPool();     // 销毁所有连接

    // 单例模式
    static connection_pool *GetInstance();  // 获取连接池实例
    void init(string url, string User, string PassWord, string DatabaseName, int Port, int MaxConn, int close_log); // 初始化

};

class connectionRAII
{
private:
    MYSQL *connRAII;
    connection_pool *poolRAII;
public:
    connectionRAII(MYSQL **conn, connection_pool *connPool);
    ~connectionRAII();
};

#endif