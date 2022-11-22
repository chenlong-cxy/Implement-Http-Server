#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "Util.hpp"
#include "Log.hpp"

#define SEP ": "
#define WEB_ROOT "wwwroot"
#define HOME_PAGE "index.html"
#define HTTP_VERSION "HTTP/1.0"
#define LINE_END "\r\n"

#define PAGE_400 "400.html"
#define PAGE_404 "404.html"
#define PAGE_500 "500.html"

#define OK 200
#define BAD_REQUEST 400
#define NOT_FOUND 404
#define INTERNAL_SERVER_ERROR 500

//根据状态码获取状态码描述
static std::string CodeToDesc(int code)
{
    std::string desc;
    switch(code){
        case 200:
            desc = "OK";
            break;
        case 400:
            desc = "Bad Request";
            break;
        case 404:
            desc = "Not Found";
            break;
        case 500:
            desc = "Internal Server Error";
            break;
        default:
            break;
    }
    return desc;
}

//根据后缀获取资源类型
static std::string SuffixToDesc(const std::string& suffix)
{
    static std::unordered_map<std::string, std::string> suffix_to_desc = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/x-javascript"},
        {".jpg", "application/x-jpg"},
        {".xml", "text/xml"}
    };
    auto iter = suffix_to_desc.find(suffix);
    if(iter != suffix_to_desc.end()){
        return iter->second;
    }
    return "text/html"; //所给后缀未找到则默认该资源为html文件
}

//HTTP请求
class HttpRequest{
    public:
        //HTTP请求内容
        std::string _request_line;                //请求行
        std::vector<std::string> _request_header; //请求报头
        std::string _blank;                       //空行
        std::string _request_body;                //请求正文

        //解析结果
        std::string _method;       //请求方法
        std::string _uri;          //URI
        std::string _version;      //版本号
        std::unordered_map<std::string, std::string> _header_kv; //请求报头中的键值对
        int _content_length;       //正文长度
        std::string _path;         //请求资源的路径
        std::string _query_string; //uri中携带的参数

        //CGI相关
        bool _cgi; //是否需要使用CGI模式
    public:
        HttpRequest()
            :_content_length(0) //默认请求正文长度为0
            ,_cgi(false)        //默认不使用CGI模式
        {}
        ~HttpRequest()
        {}
};

//HTTP响应
class HttpResponse{
    public:
        //HTTP响应内容
        std::string _status_line;                  //状态行
        std::vector<std::string> _response_header; //响应报头
        std::string _blank;                        //空行
        std::string _response_body;                //响应正文（CGI相关）

        //所需数据
        int _status_code;    //状态码
        int _fd;             //响应文件的fd  （非CGI相关）
        int _size;           //响应文件的大小（非CGI相关）
        std::string _suffix; //响应文件的后缀（非CGI相关）
    public:
        HttpResponse()
            :_blank(LINE_END) //设置空行
            ,_status_code(OK) //状态码默认为200
            ,_fd(-1)          //响应文件的fd初始化为-1
            ,_size(0)         //响应文件的大小默认为0
        {}
        ~HttpResponse()
        {}
};

//读取请求、分析请求、构建响应
//IO通信

//服务端EndPoint
class EndPoint{
    private:
        int _sock;                   //通信的套接字
        HttpRequest _http_request;   //HTTP请求
        HttpResponse _http_response; //HTTP响应
    private:
        //读取请求行
        void RecvHttpRequestLine()
        {
            auto& line = _http_request._request_line;
            if(Util::ReadLine(_sock, line) > 0){
                line.resize(line.size() - 1); //去掉读取上来的\n
            }
        }
        //读取请求报头和空行
        void RecvHttpRequestHeader()
        {
            std::string line;
            while(true){
                line.clear(); //每次读取之前清空line
                Util::ReadLine(_sock, line);
                if(line == "\n"){ //读取到了空行
                    _http_request._blank = line;
                    break;
                }
                //读取到一行请求报头
                line.resize(line.size() - 1); //去掉读取上来的\n
                _http_request._request_header.push_back(line);
            }
        }
        //解析请求行
        void ParseHttpRequestLine()
        {
            auto& line = _http_request._request_line;

            //通过stringstream拆分请求行
            std::stringstream ss(line);
            ss>>_http_request._method>>_http_request._uri>>_http_request._version;

            //将请求方法统一转换为全大写
            auto& method = _http_request._method;
            std::transform(method.begin(), method.end(), method.begin(), toupper);
        }
        //解析请求报头
        void ParseHttpRequestHeader()
        {
            std::string key;
            std::string value;
            for(auto& iter : _http_request._request_header){
                //将每行请求报头打散成kv键值对，插入到unordered_map中
                if(Util::CutString(iter, key, value, SEP)){
                    _http_request._header_kv.insert({key, value});
                }
            }
        }
        //判断是否需要读取请求正文
        bool IsNeedRecvHttpRequestBody()
        {
            auto& method = _http_request._method;
            if(method == "POST"){ //请求方法为POST则需要读取正文
                auto& header_kv = _http_request._header_kv;
                //通过Content-Length获取请求正文长度
                auto iter = header_kv.find("Content-Length");
                if(iter != header_kv.end()){
                    _http_request._content_length = atoi(iter->second.c_str());
                    return true;
                }
            }
            return false;
        }
        //读取请求正文
        void RecvHttpRequestBody()
        {
            if(IsNeedRecvHttpRequestBody()){ //先判断是否需要读取正文
                int content_length = _http_request._content_length;
                auto& body = _http_request._request_body;

                //读取请求正文
                char ch = 0;
                while(content_length){
                    ssize_t size = recv(_sock, &ch, 1, 0);
                    if(size > 0){
                        body.push_back(ch);
                        content_length--;
                    }
                    else{
                        break;
                    }
                }
            }
        }
        //CGI处理
        int ProcessCgi()
        {
            int code = OK; //要返回的状态码，默认设置为200

            auto& bin = _http_request._path;      //需要执行的CGI程序
            auto& method = _http_request._method; //请求方法

            //需要传递给CGI程序的参数
            auto& query_string = _http_request._query_string; //GET
            auto& request_body = _http_request._request_body; //POST

            int content_length = _http_request._content_length;  //请求正文的长度
            auto& response_body = _http_response._response_body; //CGI程序的处理结果放到响应正文当中

            //1、创建两个匿名管道（管道命名站在父进程角度）
            //创建从子进程到父进程的通信信道
            int input[2];
            if(pipe(input) < 0){ //管道创建失败，则返回对应的状态码
                LOG(ERROR, "pipe input error!");
                code = INTERNAL_SERVER_ERROR;
                return code;
            }
            //创建从父进程到子进程的通信信道
            int output[2];
            if(pipe(output) < 0){ //管道创建失败，则返回对应的状态码
                LOG(ERROR, "pipe output error!");
                code = INTERNAL_SERVER_ERROR;
                return code;
            }

            //2、创建子进程
            pid_t pid = fork();
            if(pid == 0){ //child
                //子进程关闭两个管道对应的读写端
                close(input[0]);
                close(output[1]);

                //将请求方法通过环境变量传参
                std::string method_env = "METHOD=";
                method_env += method;
                putenv((char*)method_env.c_str());

                if(method == "GET"){ //将query_string通过环境变量传参
                    std::string query_env = "QUERY_STRING=";
                    query_env += query_string;
                    putenv((char*)query_env.c_str());
                    LOG(INFO, "GET Method, Add Query_String env");
                }
                else if(method == "POST"){ //将正文长度通过环境变量传参
                    std::string content_length_env = "CONTENT_LENGTH=";
                    content_length_env += std::to_string(content_length);
                    putenv((char*)content_length_env.c_str());
                    LOG(INFO, "POST Method, Add Content_Length env");
                }
                else{
                    //Do Nothing
                }

                //3、将子进程的标准输入输出进行重定向
                dup2(output[0], 0); //标准输入重定向到管道的输入
                dup2(input[1], 1);  //标准输出重定向到管道的输出

                //4、将子进程替换为对应的CGI程序
                execl(bin.c_str(), bin.c_str(), nullptr);
                exit(1); //替换失败
            }
            else if(pid < 0){ //创建子进程失败，则返回对应的错误码
                LOG(ERROR, "fork error!");
                code = INTERNAL_SERVER_ERROR;
                return code;
            }
            else{ //father
                //父进程关闭两个管道对应的读写端
                close(input[1]);
                close(output[0]);

                if(method == "POST"){ //将正文中的参数通过管道传递给CGI程序
                    const char* start = request_body.c_str();
                    int total = 0;
                    int size = 0;
                    while(total < content_length && (size = write(output[1], start + total, request_body.size() - total)) > 0){
                        total += size;
                    }
                }

                //读取CGI程序的处理结果
                char ch = 0;
                while(read(input[0], &ch, 1) > 0){
                    response_body.push_back(ch);
                } //不会一直读，当另一端关闭后会继续执行下面的代码

                //等待子进程（CGI程序）退出
                int status = 0;
                pid_t ret = waitpid(pid, &status, 0);
                if(ret == pid){
                    if(WIFEXITED(status)){ //正常退出
                        if(WEXITSTATUS(status) == 0){ //结果正确
                            LOG(INFO, "CGI program exits normally with correct results");
                            code = OK;
                        }
                        else{
                            LOG(INFO, "CGI program exits normally with incorrect results");
                            code = BAD_REQUEST;
                        }
                    }
                    else{
                        LOG(INFO, "CGI program exits abnormally");
                        code = INTERNAL_SERVER_ERROR;
                    }
                }

                //关闭两个管道对应的文件描述符
                close(input[0]);
                close(output[1]);
            }
            return code; //返回状态码
        }
        //非CGI处理
        int ProcessNonCgi()
        {
            //打开客户端请求的资源文件，以供后续发送
            _http_response._fd = open(_http_request._path.c_str(), O_RDONLY);
            if(_http_response._fd >= 0){ //打开文件成功
                return OK;
            }
            return INTERNAL_SERVER_ERROR; //打开文件失败
        }

        void BuildOkResponse()
        {
            //构建响应报头
            std::string content_type = "Content-Type: ";
            content_type += SuffixToDesc(_http_response._suffix);
            content_type += LINE_END;
            _http_response._response_header.push_back(content_type);

            std::string content_length = "Content-Length: ";
            if(_http_request._cgi){ //以CGI方式请求
                content_length += std::to_string(_http_response._response_body.size());
            }
            else{ //以非CGI方式请求
                content_length += std::to_string(_http_response._size);
            }
            content_length += LINE_END;
            _http_response._response_header.push_back(content_length);
        }

        void HandlerError(std::string page)
        {
            _http_request._cgi = false; //需要返回对应的错误页面（非CGI返回）

            //打开对应的错误页面文件，以供后续发送
            _http_response._fd = open(page.c_str(), O_RDONLY);
            if(_http_response._fd > 0){ //打开文件成功
                //构建响应报头
                struct stat st;
                stat(page.c_str(), &st); //获取错误页面文件的属性信息

                std::string content_type = "Content-Type: text/html";
                content_type += LINE_END;
                _http_response._response_header.push_back(content_type);

                std::string content_length = "Content-Length: ";
                content_length += std::to_string(st.st_size);
                content_length += LINE_END;
                _http_response._response_header.push_back(content_length);

                _http_response._size = st.st_size; //重新设置响应文件的大小
            }
        }
    public:
        EndPoint(int sock)
            :_sock(sock)
        {}
        //读取请求
        void RecvHttpRequest()
        {
            RecvHttpRequestLine();    //读取请求行
            RecvHttpRequestHeader();  //读取请求报头
            ParseHttpRequestLine();   //解析请求行
            ParseHttpRequestHeader(); //解析请求报头
            RecvHttpRequestBody();    //读取请求正文
        }
        //处理请求
        void HandlerHttpRequest()
        {
            auto& code = _http_response._status_code;

            if(_http_request._method != "GET"&&_http_request._method != "POST"){ //非法请求
                LOG(WARNING, "method is not right");
                code = BAD_REQUEST; //设置对应的状态码，并直接返回
                return;
            }

            if(_http_request._method == "GET"){
                size_t pos = _http_request._uri.find('?');
                if(pos != std::string::npos){ //uri中携带参数
                    //切割uri，得到客户端请求资源的路径和uri中携带的参数
                    Util::CutString(_http_request._uri, _http_request._path, _http_request._query_string, "?");
                    _http_request._cgi = true; //上传了参数，需要使用CGI模式
                }
                else{ //uri中没有携带参数
                    _http_request._path = _http_request._uri; //uri即是客户端请求资源的路径
                }
            }
            else if(_http_request._method == "POST"){
                _http_request._path = _http_request._uri; //uri即是客户端请求资源的路径
                _http_request._cgi = true; //上传了参数，需要使用CGI模式
            }
            else{
                //Do Nothing
            }

            //给请求资源路径拼接web根目录
            std::string path = _http_request._path;
            _http_request._path = WEB_ROOT;
            _http_request._path += path;

            //请求资源路径以/结尾，说明请求的是一个目录
            if(_http_request._path[_http_request._path.size() - 1] == '/'){
                //拼接上该目录下的index.html
                _http_request._path += HOME_PAGE;
            }
            
            //获取请求资源文件的属性信息
            struct stat st;
            if(stat(_http_request._path.c_str(), &st) == 0){ //属性信息获取成功，说明该资源存在
                if(S_ISDIR(st.st_mode)){ //该资源是一个目录
                    _http_request._path += "/"; //需要拼接/，以/结尾的目录前面已经处理过了
                    _http_request._path += HOME_PAGE; //拼接上该目录下的index.html
                    stat(_http_request._path.c_str(), &st); //需要重新资源文件的属性信息
                }
                else if(st.st_mode&S_IXUSR||st.st_mode&S_IXGRP||st.st_mode&S_IXOTH){ //该资源是一个可执行程序
                    _http_request._cgi = true; //需要使用CGI模式
                }
                _http_response._size = st.st_size; //设置请求资源文件的大小
            }
            else{ //属性信息获取失败，可以认为该资源不存在
                LOG(WARNING, _http_request._path + " NOT_FOUND");
                code = NOT_FOUND; //设置对应的状态码，并直接返回
                return;
            }

            //获取请求资源文件的后缀
            size_t pos = _http_request._path.rfind('.');
            if(pos == std::string::npos){
                _http_response._suffix = ".html"; //默认设置
            }
            else{
                _http_response._suffix = _http_request._path.substr(pos);
            }

            //进行CGI或非CGI处理
            if(_http_request._cgi == true){
                code = ProcessCgi(); //以CGI的方式进行处理
            }
            else{
                code = ProcessNonCgi(); //简单的网页返回，返回静态网页
            }
        }

        //构建响应
        void BuildHttpResponse()
        {
            int code = _http_response._status_code;
            //构建状态行
            auto& status_line = _http_response._status_line;
            status_line += HTTP_VERSION;
            status_line += " ";
            status_line += std::to_string(code);
            status_line += " ";
            status_line += CodeToDesc(code);
            status_line += LINE_END;

            //构建响应报头
            std::string path = WEB_ROOT;
            path += "/";
            switch(code){
                case OK:
                    BuildOkResponse();
                    break;
                case NOT_FOUND:
                    path += PAGE_404;
                    HandlerError(path);
                    break;
                case BAD_REQUEST:
                    path += PAGE_400;
                    HandlerError(path);
                    break;
                case INTERNAL_SERVER_ERROR:
                    path += PAGE_500;
                    HandlerError(path);
                    break;
                default:
                    break;
            }
        }

        //发送响应
        void SendHttpResponse()
        {
            //发送状态行
            send(_sock, _http_response._status_line.c_str(), _http_response._status_line.size(), 0);
            //发送响应报头
            for(auto& iter : _http_response._response_header){
                send(_sock, iter.c_str(), iter.size(), 0);
            }
            //发送空行
            send(_sock, _http_response._blank.c_str(), _http_response._blank.size(), 0);
            //发送响应正文
            if(_http_request._cgi){
                auto& response_body = _http_response._response_body;
                const char* start = response_body.c_str();
                size_t size = 0;
                size_t total = 0;
                while(total < response_body.size()&&(size = send(_sock, start + total, response_body.size() - total, 0)) > 0){
                    total += size;
                }
            }
            else{
                sendfile(_sock, _http_response._fd, nullptr, _http_response._size);
                //关闭请求的资源文件
                close(_http_response._fd);
            }
        }
        ~EndPoint()
        {}
};

class CallBack{
    public:
        static void* HandlerRequest(void* arg)
        {
            LOG(INFO, "handler request begin");
            int sock = *(int*)arg;

#ifdef DEBUG
            char buffer[4096];
            recv(sock, buffer, sizeof(buffer), 0);
            std::cout<<"------------------begin------------------"<<std::endl;
            std::cout<<buffer<<std::endl;
            std::cout<<"-------------------end-------------------"<<std::endl;
#else
            EndPoint* ep = new EndPoint(sock);
            ep->RecvHttpRequest();    //读取请求
            ep->HandlerHttpRequest(); //处理请求
            ep->BuildHttpResponse();  //构建响应
            ep->SendHttpResponse();   //发送响应

            close(sock); //关闭与该客户端建立的套接字
            delete ep;
#endif
            LOG(INFO, "handler request end");
            return nullptr;
        }
};
