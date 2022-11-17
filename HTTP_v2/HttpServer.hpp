#pragma once

#include <iostream>
#include <signal.h>
#include "TcpServer.hpp"
#include "Protocol.hpp"
#include "Log.hpp"

#define PORT 8081

//HTTP服务器
class HttpServer{
    private:
        int _port; //端口号
    public:
        HttpServer(int port)
            :_port(port)
        {}

        //初始化服务器
        void InitServer()
        {
            signal(SIGPIPE, SIG_IGN); //忽略SIGPIPE信号，防止写入时崩溃
        }

        //启动服务器
        void Loop()
        {
            LOG(INFO, "loop begin");
            TcpServer* tsvr = TcpServer::GetInstance(_port); //获取TCP服务器单例对象
            int listen_sock = tsvr->Sock(); //获取监听套接字
            while(true){
                struct sockaddr_in peer;
                memset(&peer, 0, sizeof(peer));
                socklen_t len = sizeof(peer);
                int sock = accept(listen_sock, (struct sockaddr*)&peer, &len); //获取新连接
                if(sock < 0){
                    continue; //获取失败，继续获取
                }

                //打印客户端相关信息
                std::string client_ip = inet_ntoa(peer.sin_addr);
                int client_port = ntohs(peer.sin_port);
                LOG(INFO, "get a new link: ["+client_ip+":"+std::to_string(client_port)+"]");
                
                //创建新线程处理新连接发起的HTTP请求
                int* p = new int(sock);
                pthread_t tid;
                pthread_create(&tid, nullptr, CallBack::HandlerRequest, (void*)p);
                pthread_detach(tid); //线程分离
            }
        }
        ~HttpServer()
        {}
};
