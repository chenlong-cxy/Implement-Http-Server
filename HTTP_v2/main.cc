#include <iostream>
#include <string>
#include <memory>
#include "HttpServer.hpp"

static void Usage(std::string proc)
{
    std::cout<<"Usage:\n\t"<<proc<<" port"<<std::endl;
}
int main(int argc, char* argv[])
{
    if(argc != 2){
        Usage(argv[0]);
        exit(4);
    }
    int port = atoi(argv[1]); //端口号
    std::shared_ptr<HttpServer> svr(new HttpServer(port)); //创建HTTP服务器对象
    svr->InitServer(); //初始化服务器
    svr->Loop(); //启动服务器
    return 0;
}
