# MyWebServer
C++轻量级Web服务器，可以访问服务器实现web端用户**注册、登录**功能，可以请求服务器**图片和视频文件**

[caffey.icu:8888](http://caffey.icu:8888/)

requirements
-------------
* Ubuntu 20.04
* MySQL >= 6.0

* 建立数据库
  ```
  create database yourdb;

  USE yourdb;

  CREATE TABLE user(
    username char(50) NULL,
    passwd char(50) NULL
  )ENGINE=InnoDB;
  ```
* build
  ```
  sh ./build.sh
  ```
* 启动server
  ```
  ./server
  ```
  或者
  ```
  ./server -p 9007 -l 1 -m 0 -o 1 -s 10 -t 10 -c 1 -a 1
  ```
  * -p，自定义端口号
	  * 默认8888
  * -l，日志写入方式，默认0
	  * 0，同步写入
	  * 1，异步写入
  * -m，listenfd和connfd的模式组合，默认0
	  * 0，表示使用LT + LT
	  * 1，表示使用LT + ET
    * 2，表示使用ET + LT
    * 3，表示使用ET + ET
  * -o，优雅关闭连接，默认0
	  * 0，不使用
	  * 1，使用
  * -s，数据库连接数量，默认为8
  * -t，线程数量，默认为8
  * -c，关闭日志，默认0
	  * 0，打开日志
	  * 1，关闭日志
  * -a，选择反应堆模型，默认0
	  * 0，Proactor模型
	  * 1，Reactor模型


TODO List:
- 增加压力测试
- 修改timer中冗余部分
- 修改log
- 把webserver和timer的一些功能整理出Utils类，重写

写在最后
----------
实习的时候想着把实习的算法研究成果整理起来，然后扔到服务器展示一下，但是涉及实习开发项目的隐私，就只能作罢。但是WebServer还是扔上来吧，也是一种学习总结。

感谢参考[@qinguoyi](https://github.com/qinguoyi/TinyWebServer/tree/master)
