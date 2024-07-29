#include <iostream>
#include <string>

#include "config.h"

using namespace std;

int main(int argc, char *argv[]) 
{
    // 需要修改的数据库信息,登录名,密码,库名
    string username = "root";
    string password = "Cjh@0103";
    string databasename = "yourdb";

    // 命令行输入
    Config config;
    config.parse_arg(argc, argv);

    WebServer server;

    // 初始化
    server.init(config.PORT, username, password, databasename, 
                config.LOGWrite, config.OPT_LINGER, config.TRIGMode,
                config.sql_num, config.thread_num, config.close_log, config.actor_model);
    cout << "config port: " << config.PORT << endl;
    // 日志
    server.log_write();

    // 数据库
    server.sql_pool();

    // 线程池
    server.thread_pool();

    // 触发模式
    server.trig_mode();

    // 监听
    server.eventListen();

    // 运行
    server.eventLoop();

    return 0;
}