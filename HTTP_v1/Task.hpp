#pragma once

#include <iostream>
#include <unistd.h>
#include "Protocol.hpp"

class Task{
    private:
        int _sock;
        CallBack _handler; //回调
    public:
        Task()
        {}
        Task(int sock)
            :_sock(sock)
        {}
        //处理任务
        void ProcessOn()
        {
            _handler(_sock); //调用CallBack的仿函数
        }
        ~Task()
        {}
};
