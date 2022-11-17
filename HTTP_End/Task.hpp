#pragma once

#include <iostream>
#include <unistd.h>
#include "Protocol.hpp"

//任务类
class Task{
    private:
        int _sock;         //通信的套接字
        CallBack _handler; //回调函数
    public:
        Task()
        {}
        Task(int sock)
            :_sock(sock)
        {}
        //处理任务
        void ProcessOn()
        {
            _handler(_sock); //调用回调
        }
        ~Task()
        {}
};
