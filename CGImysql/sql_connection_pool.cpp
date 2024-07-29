#include <mysql/mysql.h>
#include <iostream>
#include <string>
#include <string.h>
#include <stdio.h>
#include <list>
#include <pthread.h>
#include <stdlib.h>

#include "sql_connection_pool.h"

using namespace std;

connection_pool::connection_pool()
{
    // 初始化，当前已使用的连接数为0
    m_CurConn = 0;
    m_FreeConn = 0;
}
connection_pool *connection_pool::GetInstance()
{
    static connection_pool connPool;
    return &connPool;
}

// 构造初始化
void connection_pool::init(string url, string User, string PassWord, string DatabaseName, int Port, int MaxConn, int close_log)
{
    m_url = url;
    m_Port = Port;
    m_User = User;
    m_PassWord = PassWord;
    m_DatabaseName = DatabaseName;
    m_close_log = close_log;

    // 创建MaxConn条数据库连接
    for (auto i = 0; i < MaxConn; i++)
    {
        MYSQL *con = NULL;
        con = mysql_init(con);
        // 如果数据库链接创建失败，输出错误信息
        if (con == NULL)
        {
            // TODO: LOG_ERROR("MySQL Error!");
            // LOG_ERROR("MySQL Error!");
            exit(1);
        }
        con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DatabaseName.c_str(), Port, NULL, 0);
        if (con == NULL)
        {
            // TODO: LOG_ERROR("MySQL Error!");
            // LOG_ERROR("MySQL Error!");
            exit(1);
        }
        connList.push_back(con);
        ++m_FreeConn;
    }
    // 创建可以连接数量的信号量
    reserver = sem(m_FreeConn);

    m_MaxConn = m_FreeConn;
}

// 从连接池中获取一个可用连接，更新使用和空闲连接数
MYSQL *connection_pool::GetConnection()
{
    MYSQL *con = NULL;
    if (connList.empty())
    {
        return NULL;
    }
    // if (connList.size() == 0)
    // {
    //     return NULL;
    // }
    reserver.wait();
    lock.lock();

    con = connList.front();
    connList.pop_front();

    --m_FreeConn;
    ++m_CurConn;

    lock.unlock();
    return con;
}

// 释放当前连接
bool connection_pool::ReleaseConnection(MYSQL *con)
{
    if (con == NULL)
    {
        return false;
    }
    lock.lock();
    // 将连接放回连接池
    connList.push_back(con);
    ++m_FreeConn;
    --m_CurConn;

    lock.unlock();
    reserver.post();
    return true;
}

// 获取当前空闲连接数
int connection_pool::GetFreeConn()
{
    return this->m_FreeConn;
}

// 销毁所有连接
void connection_pool::DestroyPool()
{
    lock.lock();
    if (connList.size() > 0)
    {
        for (list<MYSQL *>::iterator it = connList.begin(); it != connList.end(); ++it)
        {
            MYSQL *con = *it;
            mysql_close(con);
        }
        m_FreeConn = 0;
        m_CurConn = 0;
        // 清空连接池列表
        connList.clear();
    }
    lock.unlock();
}

// 析构函数 销毁连接池
connection_pool::~connection_pool()
{
    DestroyPool();
}

// RAII机制，自动释放连接
connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool)
{
    *SQL = connPool->GetConnection();
    // 保存连接池地址
    connRAII = *SQL;
    poolRAII = connPool;
}

connectionRAII::~connectionRAII()
{
    // 释放连接
    poolRAII->ReleaseConnection(connRAII);
}