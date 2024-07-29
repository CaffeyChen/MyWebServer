#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#include "../lock/locker.h"

using namespace std;

template <class T>
class block_queue
{
private:
    locker m_mutex;     // 互斥锁
    cond m_cond;        // 条件变量
    
    // TODO: 可以改成deque
    T *m_array;         // 队列数组 （是否可以用List/queue？）
    int m_max_size;     // 队列最大容量
    int m_size;         // 队列当前容量
    int m_front;        // 队头
    int m_back;         // 队尾

public:
    block_queue(int max_size = 1000)
    {
        if (max_size <= 0)
        {
            exit(-1);
        }
        m_max_size = max_size;
        // 初始化队列数组
        m_array = new T[max_size];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }

    void clear()
    {
        m_mutex.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_mutex.unlock();
    }

    ~block_queue()
    {
        m_mutex.lock();
        if (m_array != NULL)
        {
            delete[] m_array;
        }
        m_mutex.unlock();
    }

    bool full()
    {
        m_mutex.lock();
        if (m_size >= m_max_size)
        {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    bool empty()
    {
        m_mutex.lock();
        if (m_size == 0)
        {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    bool front(T &value)
    {
        m_mutex.lock();
        if (m_size == 0)
        {
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_front];
        m_mutex.unlock();
        return true;
    }

    bool back(T &value)
    {
        m_mutex.lock();
        if (m_size == 0)
        {
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_back];
        m_mutex.unlock();
        return true;
    }

    // 返回队列当前容量
    int size()
    {
        int tmp = 0;
        m_mutex.lock();
        tmp = m_size;
        m_mutex.unlock();
        return tmp;
    }

    int max_size()
    {
        int tmp = 0;
        m_mutex.lock();
        tmp = m_max_size;
        m_mutex.unlock();
        return tmp;
    }

    // 往队列添加元素， 需要将所有使用队列的线程先唤醒
    // 当有元素push进队列，相当于生产者生产了一个元素，消费者要消费
    // 如果当前没有线程等待，则唤醒无意义/无需唤醒
    bool push(const T &item)
    {
        m_mutex.lock();
        if (m_size >= m_max_size)
        {
            // 如果队列满了, 广播唤醒所有等待线程
            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }

        m_back = (m_back + 1) % m_max_size;
        m_array[m_back] = item;
        m_size++;

        // 唤醒等待线程
        m_cond.broadcast();
        m_mutex.unlock();
        return true;
    }

    // pop时，如果队列为空，则等待
    bool pop(T &item)
    {
        m_mutex.lock();
        while (m_size <= 0)
        {
            // 条件变量等待 在mutex lock条件下等待
            if (!m_cond.wait(m_mutex.get()))
            {
                m_mutex.unlock();
                return false;
            }
        }

        m_front = (m_front + 1) % m_max_size;
        m_size--;
        item = m_array[m_front];
        m_mutex.unlock();
        return true;
    }

    // pop 增加时间限制
    // TODO 感觉有问题可能需要修改
    bool pop(T &item, int ms_timeout)
    {
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now, NULL);

        m_mutex.lock();
        if (m_size <= 0)
        {
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            if (!m_cond.timewait(m_mutex.get(), t))
            {
                m_mutex.unlock();
                return false;
            }
        }
        if (m_size <= 0)
        {
            m_mutex.unlock();
            return false;
        }
        
        m_front = (m_front + 1) % m_max_size;
        m_size--;
        item = m_array[m_front];
        m_mutex.unlock();
        return true;
    }

};

#endif