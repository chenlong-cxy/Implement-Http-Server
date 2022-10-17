#pragma once

#include <iostream>
#include <signal.h>
#include "TcpServer.hpp"
#include "Task.hpp"
#include "ThreadPool.hpp"
#include "Log.hpp"

#define PORT 8081

class HttpServer{
    private:
        int _port;
        bool _stop;
    public:
        HttpServer(int port)
            :_port(port)
            ,_stop(false)
        {}
        void InitServer()
        {
            signal(SIGPIPE, SIG_IGN); //忽略SIGPIPE信号，防止写入时崩溃
        }
        void Loop()
        {
            LOG(INFO, "loop begin");
            TcpServer* tsvr = TcpServer::GetInstance(_port);
            int listen_sock = tsvr->Sock();
            while(!_stop){
                struct sockaddr_in peer;
                memset(&peer, 0, sizeof(peer));
                socklen_t len = sizeof(peer);
                int sock = accept(listen_sock, (struct sockaddr*)&peer, &len);
                if(sock < 0){
                    continue;
                }
                LOG(INFO, "get a new link");
                Task task(sock);
                ThreadPool::GetInstance()->PushTask(task);

                //int* p = new int(sock);
                //pthread_t tid;
                //pthread_create(&tid, nullptr, Entrance::HandlerRequest, (void*)p);
                //pthread_detach(tid);
            }
        }
        ~HttpServer()
        {}
};
