#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"


template <typename T>
class threadpool
{
private:
    int m_thread_number;            // 线程池中的线程数
    int m_max_requests;             // 请求队列中允许的最大请求数
    pthread_t *m_threads;           // 描述线程池的数组，其大小为m_thread_number
    std::list<T *> m_workqueue;     // 请求队列
    locker m_queuelocker;           // 保护请求队列的互斥锁
    sem m_queuestat;                // 是否有任务需要处理
    connection_pool *m_connPool;    // 数据库连接池
    int m_actor_model;              // 并发模型选择

    static void *worker(void *arg); // 工作线程运行的函数，不断从请求队列中取出任务并执行
    void run();

public:
    // 线程池创建
    // actor_model: 并发模型选择
    // connPool: 数据库连接池
    // thread_number: 线程池内的线程数量
    // max_request: 请求队列中允许的最大请求数量
    threadpool(int actor_model, connection_pool *connPool, int thread_number = 8, int max_request = 10000);
    
    ~threadpool();
    // 向请求队列中添加任务
    bool append(T *request, int state);
    bool append_p(T *request);
};

template <typename T>
threadpool<T>::threadpool(int actor_model, connection_pool *connPool, int thread_number, int max_request)
: m_actor_model(actor_model), m_thread_number(thread_number), m_max_requests(max_request), m_threads(NULL), m_connPool(connPool)
{
    // 如果线程数或请求数量小于等于0，则抛出异常
    if (thread_number <= 0 || max_request <= 0)
        throw std::exception();
    
    m_threads = new pthread_t[m_thread_number];
    // 如果线程创建失败，则抛出异常
    if (!m_threads)
        throw std::exception();

    for (auto i = 0; i < thread_number; ++i)
    {
        // 创建线程失败，则删除线程数组并抛出异常
        if (pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            delete[] m_threads;
            throw std::exception();
        }
        // 设置线程分离
        if (pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
}

template <typename T>
bool threadpool<T>::append(T *request, int state)
{
    m_queuelocker.lock();
    if (m_workqueue.size() >= m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    request->m_state = state;
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    // 状态信号量+1
    m_queuestat.post();
    return true;
}

template <typename T>
bool threadpool<T>::append_p(T *request)
{
    m_queuelocker.lock();
    if (m_workqueue.size() >= m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

template <typename T>
void *threadpool<T>::worker(void *arg)
{
    // 将参数强转为线程池类
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}

template <typename T>
void threadpool<T>::run()
{
    while (true)
    {
        // 等待状态信号量
        m_queuestat.wait();
        m_queuelocker.lock();
        if (m_workqueue.empty())
        {
            m_queuelocker.unlock();
            continue;
        }
        // 从请求队列中取出第一个任务
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if (!request)
        {
            continue;
        }

        if (m_actor_model == 1)
        {
            // 请求是0，则为读
            if (request->m_state == 0)
            {
                // 如果读取一次成功，则将请求加入到mysql连接池中
                if (request->read_once())
                {
                    request->improv = 1;
                    connectionRAII mysqlcon(&request->mysql, m_connPool);
                    request->process();
                }
                else
                {
                    // 读取失败
                    request->improv = 1;
                    request->timer_flag = 1;
                }
                
            }
            else
            {
                // 写
                if (request->write())
                {
                    request->improv = 1;
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
                
            }
            
        }
        else
        {
            connectionRAII mysqlcon(&request->mysql, m_connPool);
            request->process();
        }   
    }   
}

#endif