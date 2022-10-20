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

class TcpServer{
    private:
        int _port; //端口号
        int _listen_sock; //监听套接字
        static TcpServer* _svr;
    private:
        TcpServer(int port)
            :_port(port)
            ,_listen_sock(-1)
        {}
        TcpServer(const TcpServer&)
        {}
    public:
        static TcpServer* GetInstance(int port)
        {
            static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
            if(_svr == nullptr){
                pthread_mutex_lock(&lock);
                if(_svr == nullptr){
                    _svr = new TcpServer(port);
                    _svr->InitServer();
                }
                pthread_mutex_unlock(&lock);
            }
            return _svr;
        }
        void InitServer()
        {
            Socket();
            Bind();
            Listen();
            LOG(INFO, "tcp_server init ... success");
        }
        void Socket()
        {
            _listen_sock = socket(AF_INET, SOCK_STREAM, 0);
            if(_listen_sock < 0){
                LOG(FATAL, "socket error!");
                exit(1);
            }
            int opt = 1;
            setsockopt(_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            LOG(INFO, "create socket ... success");
        }
        void Bind()
        {
            struct sockaddr_in local;
            memset(&local, 0, sizeof(local));
            local.sin_family = AF_INET;
            local.sin_port = htons(_port);
            local.sin_addr.s_addr = INADDR_ANY; //云服务器不能直接绑定公网IP
            if(bind(_listen_sock, (struct sockaddr*)&local, sizeof(local)) < 0){
                LOG(FATAL, "bind error!");
                exit(2);
            }
            LOG(INFO, "bind socket ... success");
        }
        void Listen()
        {
            if(listen(_listen_sock, BACKLOG) < 0){
                LOG(FATAL, "listen error!");
                exit(3);
            }
            LOG(INFO, "listen socket ... success");
        }
        int Sock()
        {
            return _listen_sock;
        }
        ~TcpServer()
        {
            if(_listen_sock >= 0){
                close(_listen_sock);
            }
        }
};
TcpServer* TcpServer::_svr = nullptr;
