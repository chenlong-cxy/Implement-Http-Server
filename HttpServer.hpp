#pragma once

#include <iostream>
#include <signal.h>
#include "TcpServer.hpp"
#include "Protocol.hpp"
#include "Log.hpp"

#define PORT 8081

class HttpServer{
    private:
        int _port;
        TcpServer* _tcp_server;
        bool _stop;
    public:
        HttpServer(int port)
            :_port(port)
            ,_tcp_server(nullptr)
            ,_stop(false)
        {}
        void InitServer()
        {
            signal(SIGPIPE, SIG_IGN); //忽略SIGPIPE信号，防止写入时崩溃
            _tcp_server = TcpServer::GetInstance(_port);
        }
        void Loop()
        {
            LOG(INFO, "loop begin");
            int listen_sock = _tcp_server->Sock();
            while(!_stop){
                struct sockaddr_in peer;
                memset(&peer, 0, sizeof(peer));
                socklen_t len = sizeof(peer);
                int sock = accept(listen_sock, (struct sockaddr*)&peer, &len);
                if(sock < 0){
                    continue;
                }
                LOG(INFO, "get a new link");
                int* p = new int(sock);
                pthread_t tid;
                pthread_create(&tid, nullptr, Entrance::HandlerRequest, (void*)p);
                pthread_detach(tid);
            }
        }
        ~HttpServer()
        {}
};
