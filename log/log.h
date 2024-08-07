#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include <sys/stat.h> 
#include "block_queue.h"

using namespace std;

class Log
{
public:
    static Log *get_instance()
    {
        static Log instance;
        return &instance;
    }
    // 异步写日志公有方法，调用私有方法async_write_log
    static void *flush_log_thread(void *args)
    {
        Log::get_instance()->async_write_log();
    }
    // 初始化 日志文件名，关闭日志，日志缓冲区大小，日志最大行数，阻塞队列大小
    bool init(const char *file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);
    // 写日志
    void write_log(int level, const char *format, ...);
    // 强制刷新缓冲区
    void flush(void);

private:
    char dir_name[128];     // 路径名
    char log_name[128];     // log文件名
    int m_split_lines;      // 日志最大行数
    int m_log_buf_size;     // 日志缓冲区大小
    long long m_count;      // 日志行数记录
    int m_today;            // 因为按天分类，记录当前时间是哪一天
    FILE *m_fp;             // 打开log的文件指针
    char *m_buf;            // 写缓冲区
    block_queue<string> *m_log_queue; // 阻塞队列
    bool m_is_async;        // 是否异步标志位
    locker m_mutex;         // 互斥锁
    int m_close_log;        // 关闭日志

    Log();
    virtual ~Log();
    void *async_write_log()
    {
        string single_log;
        // 从阻塞队列中取出一个日志string, 写入文件
        while (m_log_queue->pop(single_log))
        {
            m_mutex.lock();
            fputs(single_log.c_str(), m_fp);
            m_mutex.unlock();
        }
    }
};

#define LOG_BASE(level, format, ...) if (m_close_log == 0) {Log::get_instance()->write_log(level, format, ##__VA_ARGS__); Log::get_instance()->flush();}

#define LOG_DEBUG(format, ...) LOG_BASE(0, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) LOG_BASE(1, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) LOG_BASE(2, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) LOG_BASE(3, format, ##__VA_ARGS__)

#endif