#ifndef HTTP_CONN_H
#define HTTP_CONN_H

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
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unordered_map>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../log/log.h"

class http_conn
{
public:
    static const int FILENAME_LEN = 200;        // 文件名的最大长度
    static const int READ_BUFFER_SIZE = 2048;   // 读缓冲区的大小
    static const int WRITE_BUFFER_SIZE = 1024;  // 写缓冲区的大小
    enum METHOD                                 // HTTP请求方法
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE                            // 解析客户请求时，主状态机所处的状态
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE                              // 服务器处理HTTP请求的可能结果
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS                            // 行的读取状态
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    // 初始化新接受的连接
    void init(int sockfd, const sockaddr_in &addr, char *, int, int, string user, string passwd, string sqlname);
    // 关闭连接
    void close_conn(bool real_colse = true);
    void process();
    bool read_once();
    bool write();
    // 获取socket地址
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);

    int timer_flag;     // 定时标志
    int improv;         // 是否改善连接

private:
    void init();
    // 解析HTTP请求
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line()
    {
        return m_read_buf + m_start_line;
    }
    LINE_STATUS parse_line();
    void unmap();                               // 释放资源
    bool add_response(const char *format, ...);  // 向m_write_buf中写入响应报文数据
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);    // 添加状态行
    bool add_headers(int content_length);                   // 添加响应报文头
    bool add_content_length(int content_length);            // 添加Content-Length
    bool add_linger();                                      // 添加Connection
    bool add_blank_line();                                  // 添加空行

public:
    static int m_epollfd;
    static int m_user_count;
    MYSQL *mysql;
    int m_state;                            // 读为0， 写为1 

private:
    int m_sockfd;
    sockaddr_in m_address;
    char m_read_buf[READ_BUFFER_SIZE];      // 读缓冲区
    long m_read_idx;                        // 标识读缓冲区中已经读入的客户数据的最后一个字节的下一个位置
    long m_check_idx;                       // 当前正在分析的字符在读缓冲区中的位置
    int m_start_line;                       // 当前正在解析的行的起始位置
    
    char m_write_buf[WRITE_BUFFER_SIZE];    // 写缓冲区
    int m_write_idx;                        // 写缓冲区中待发送的字节数
    CHECK_STATE m_check_state;              // 主状态机当前所处的状态
    METHOD m_method;                        // 请求方法
    char m_real_file[FILENAME_LEN];         // 客户请求的目标文件的完整路径

    char *m_url;                            // 客户请求的目标文件的文件名
    char *m_version;                        // HTTP协议版本号， 我们仅支持HTTP/1.1
    char *m_host;                           // 主机名
    long m_content_length;  
    bool m_linger;                          // HTTP请求是否要求保持连接
    char *m_file_address;   

    struct stat m_file_stat;
    struct iovec m_iv[2];                   // 采用writev来执行写操作，这是一个io向量机制
    int m_iv_count;

    int cgi;                                // 是否启用的POST
    char *m_string;                         // 存储请求头数据
    int bytes_to_send;                      // 剩余发送字节数
    int bytes_have_send;                    // 已发送字节数
    char *doc_root;                         // 文档根目录

    unordered_map<string, string> m_users;  // 用户名和密码
    int m_TRIGMode;
    int m_close_log;

    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];
};

#endif