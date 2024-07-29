#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <pthread.h>

#include "log.h"

using namespace std;

Log::Log()
{
    // 日志行数为0
    m_count = 0;
    // 设定为同步
    m_is_async = false;
}

Log::~Log()
{
    // 关闭文件
    if(m_fp != NULL)
    {
        fclose(m_fp);
    }
}

// TODO: 感觉可以换c++的string写法
// 异步需要设置阻塞队列的长度，同步不需要
bool Log::init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size)
{
    // 如果设置了阻塞队列最大长度，则将日志设置为异步
    if (max_queue_size >= 1)
    {
        m_is_async = true;
        // 初始化阻塞队列
        m_log_queue = new block_queue<string>(max_queue_size);
        // 创建线程，将日志写入文件
        pthread_t tid;
        pthread_create(&tid, NULL, flush_log_thread, NULL);
    }
    m_close_log = close_log;
    m_log_buf_size = log_buf_size;
    m_split_lines = split_lines;
    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', m_log_buf_size);

    // 获取当前时间
    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    // 获取'/'及之后的字符串
    const char *p = strrchr(file_name, '/');
    char log_full_name[256] = {0};

    // 如果没有'/'，则直接将时间+文件名赋值给log_full_name
    if (p == NULL)
    {
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    }
    else
    {
        // 将'/'之后的字符串拷贝到log_name中
        strcpy(log_name, p + 1);
        // 将'/'之前的字符串拷贝到dir_name中
        strncpy(dir_name, file_name, p - file_name + 1);
        // 将'/'之前的字符串拼接到log_full_name中
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }

    m_today = my_tm.tm_mday;

    m_fp = fopen(log_full_name, "a");
    if (m_fp == NULL)
    {
        return false;
    }
    return true;
}

void Log::flush(void)
{
    // 强制刷新写入流缓冲区
    m_mutex.lock();
    fflush(m_fp);
    m_mutex.unlock();
}

void Log::write_log(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;

    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    
    char s[16] = {0};
    switch (level)
    {
        case 0:
            strcpy(s, "[debug]:");
            break;
        case 1:
            strcpy(s, "[info]:");
            break;
        case 2:
            strcpy(s, "[warn]:");
            break;
        case 3:
            strcpy(s, "[error]:");
            break;
        default:
            strcpy(s, "[info]:");
            break;
    }

    // 写入一行日志
    m_mutex.lock();

    m_count++;
    // 如果是新的一天或者日志行数达到最大值，则重新创建文件
    if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0)
    {
        char new_log[256] = {0};
        // 关闭当前文件
        fflush(m_fp);
        fclose(m_fp);

        char tail[16] = {0};
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (m_today != my_tm.tm_mday)
        {
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        }
        else
        {
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }
        m_fp = fopen(new_log, "a");
    }

    m_mutex.unlock();

    // 
    va_list valst;
    va_start(valst, format);

    string log_str;
    m_mutex.lock();

    // 写入时间
    int n = snprintf(m_buf, 48, "%d_%02d_%02d %02d:%02d:%02d.%06ld %s ", my_tm.tm_year + 1900,
                     my_tm.tm_mon + 1, my_tm.tm_mday, my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    int m = vsnprintf(m_buf + n, m_log_buf_size - 1, format, valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';

    m_mutex.unlock();

    if (m_is_async && !m_log_queue->full())
    {
        m_log_queue->push(string(m_buf));
    }
    else
    {
        m_mutex.lock();
        fputs(m_buf, m_fp);
        m_mutex.unlock();
    }

    va_end(valst);


}