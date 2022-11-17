#pragma once

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "Log.hpp"

#define BACKLOG 5

//TCP服务器
class TcpServer{
    private:
        int _port;              //端口号
        int _listen_sock;       //监听套接字
        static TcpServer* _svr; //指向单例对象的static指针
    private:
        //构造函数私有
        TcpServer(int port)
            :_port(port)
            ,_listen_sock(-1)
        {}
        //将拷贝构造函数和拷贝赋值函数私有或删除（防拷贝）
        TcpServer(const TcpServer&)=delete;
        TcpServer* operator=(const TcpServer&)=delete;

    public:
        //获取单例对象
        static TcpServer* GetInstance(int port)
        {
            static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; //定义静态的互斥锁
            if(_svr == nullptr){
                pthread_mutex_lock(&mtx); //加锁
                if(_svr == nullptr){
                    //创建单例TCP服务器对象并初始化
                    _svr = new TcpServer(port);
                    _svr->InitServer();
                }
                pthread_mutex_unlock(&mtx); //解锁
            }
            return _svr; //返回单例对象
        }

        //初始化服务器
        void InitServer()
        {
            Socket(); //创建套接字
            Bind();   //绑定
            Listen(); //监听
            LOG(INFO, "tcp_server init ... success");
        }

        //创建套接字
        void Socket()
        {
            _listen_sock = socket(AF_INET, SOCK_STREAM, 0);
            if(_listen_sock < 0){ //创建套接字失败
                LOG(FATAL, "socket error!");
                exit(1);
            }
            //设置端口复用
            int opt = 1;
            setsockopt(_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            LOG(INFO, "create socket ... success");
        }

        //绑定
        void Bind()
        {
            struct sockaddr_in local;
            memset(&local, 0, sizeof(local));
            local.sin_family = AF_INET;
            local.sin_port = htons(_port);
            local.sin_addr.s_addr = INADDR_ANY;

            if(bind(_listen_sock, (struct sockaddr*)&local, sizeof(local)) < 0){ //绑定失败
                LOG(FATAL, "bind error!");
                exit(2);
            }
            LOG(INFO, "bind socket ... success");
        }

        //监听
        void Listen()
        {
            if(listen(_listen_sock, BACKLOG) < 0){ //监听失败
                LOG(FATAL, "listen error!");
                exit(3);
            }
            LOG(INFO, "listen socket ... success");
        }

        //获取监听套接字
        int Sock()
        {
            return _listen_sock;
        }

        ~TcpServer()
        {
            if(_listen_sock >= 0){ //关闭监听套接字
                close(_listen_sock);
            }
        }
};
//单例对象指针初始化为nullptr
TcpServer* TcpServer::_svr = nullptr;

