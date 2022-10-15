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
//#define DEBUG 1

#define OK 200
#define NOT_FOUND 404

static std::string CodeToDesc(int code)
{
    std::string desc;
    switch(code){
        case 200:
            desc = "OK";
            break;
        case 404:
            desc = "Not Found";
            break;
        default:
            break;
    }
    return desc;
}
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
    return "text/html";
}

class HttpRequest{
    public:
        std::string _request_line; //请求行
        std::vector<std::string> _request_header; //请求报头
        std::string _blank; //空行
        std::string _request_body; //请求正文

        //解析完毕之后的结果
        std::string _method; //请求方法
        std::string _uri; //URI
        std::string _version; //版本号

        std::unordered_map<std::string, std::string> _header_kv; //请求报头中的键值对
        int _content_length; //正文长度
        std::string _path; //请求资源的路径
        std::string _query_string; //uri中携带的参数
        bool _cgi; //是否需要使用CGI模式
    public:
        HttpRequest()
            :_content_length(0)
            ,_cgi(false)
        {}
        ~HttpRequest()
        {}
};

class HttpResponse{
    public:
        std::string _status_line; //状态行
        std::vector<std::string> _response_header; //响应报头
        std::string _blank; //空行
        std::string _response_body; //响应正文

        int _status_code; //状态码
        int _fd; //响应的文件
        int _size; //响应文件的大小
        std::string _suffix; //响应文件的后缀
    public:
        HttpResponse()
            :_blank(LINE_END)
            ,_status_code(OK)
            ,_fd(-1)
            ,_size(0)
        {}
        ~HttpResponse()
        {}
};

//读取请求、分析请求、构建响应
//IO通信
class EndPoint{
    private:
        int _sock;
        HttpRequest _http_request;
        HttpResponse _http_response;
    private:
        //读取请求行
        void RecvHttpRequestLine()
        {
            auto& line = _http_request._request_line;
            Util::ReadLine(_sock, line);
            line.resize(line.size() - 1);
            LOG(INFO, line);
        }
        //读取请求报头和空行
        void RecvHttpRequestHeader()
        {
            std::string line;
            while(true){
                line.clear(); //每次读取之前清空line
                Util::ReadLine(_sock, line);
                if(line == "\n"){
                    _http_request._blank = line;
                    break;
                }
                line.resize(line.size() - 1);
                _http_request._request_header.push_back(line);
                LOG(INFO, line);
            }
        }
        //解析请求行
        void ParseHttpRequestLine()
        {
            auto& line = _http_request._request_line;
            std::stringstream ss(line);
            ss>>_http_request._method>>_http_request._uri>>_http_request._version;
            auto& method = _http_request._method;
            std::transform(method.begin(), method.end(), method.begin(), toupper);
        }
        //解析请求报头
        void ParseHttpRequestHeader()
        {
            std::string key;
            std::string value;
            for(auto& iter : _http_request._request_header){
                if(Util::CutString(iter, key, value, SEP))
                {
                    _http_request._header_kv.insert({key, value});
                }
            }
        }
        //判定是否需要读取请求正文
        bool IsNeedRecvHttpRequestBody()
        {
            auto& method = _http_request._method;
            if(method == "POST"){
                auto& header_kv = _http_request._header_kv;
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
            if(IsNeedRecvHttpRequestBody()){
                int content_length = _http_request._content_length;
                auto& body = _http_request._request_body;

                char ch = 0;
                while(content_length){
                    ssize_t size = recv(_sock, &ch, 1, 0);
                    if(size > 0){
                        body.push_back(ch);
                        content_length--;
                    }
                    else if(size == 0){
                        break;
                    }
                    else{
                        break;
                    }
                }
                std::cout<<"debug: "<<body<<std::endl;
            }
        }
        //非CGI处理
        int ProcessNonCgi()
        {
            //打开待发送的文件
            _http_response._fd = open(_http_request._path.c_str(), O_RDONLY);
            if(_http_response._fd >= 0){ //打开文件成功再构建
                //构建状态行
                _http_response._status_line = HTTP_VERSION;
                _http_response._status_line += " ";
                _http_response._status_line += std::to_string(_http_response._status_code);
                _http_response._status_line += " ";
                _http_response._status_line += CodeToDesc(_http_response._status_code);
                _http_response._status_line += LINE_END;
                //构建响应报头
                std::string content_type = "Content-Type: ";
                content_type += SuffixToDesc(_http_response._suffix);
                content_type += LINE_END;
                _http_response._response_header.push_back(content_type);

                std::string content_length = "Content-Length: ";
                content_length += std::to_string(_http_response._size);
                content_length += LINE_END;
                _http_response._response_header.push_back(content_length);
                return OK;
            }
            return 404;
        }
        //CGI处理
        int ProcessCgi()
        {
            auto& bin = _http_request._path; //要让子进程执行的目标程序
            auto& method = _http_request._method;
            //父进程的数据
            auto& query_string = _http_request._query_string; //GET
            auto& request_body = _http_request._request_body; //POST

            //站在父进程角度
            int input[2];
            int output[2];
            if(pipe(input) < 0){
                LOG(ERROR, "pipe input error!");
                return 404;
            }
            if(pipe(output) < 0){
                LOG(ERROR, "pipe output error!");
                return 404;
            }

            pid_t pid = fork();
            if(pid == 0){ //child
                close(input[0]);
                close(output[1]);

                dup2(output[0], 0);
                dup2(input[1], 1);

                execl(bin.c_str(), bin.c_str(), nullptr);
                exit(1);
            }
            else if(pid < 0){
                LOG(ERROR, "fork error!");
                return 404;
            }
            else{ //father
                close(input[1]);
                close(output[0]);
                if(method == "POST"){ //将数据写入到管道当中
                    const char* start = request_body.c_str();
                    int total = 0;
                    int size = 0;
                    while((size = write(output[1], start + total, request_body.size() - total)) > 0){
                        total += size;
                    }
                }
                //std::string test_string = "2021dragon";
                //send(output[1], test_string.c_str(), test_string.size(), 0);

                waitpid(pid, nullptr, 0);

                //释放文件描述符
                close(input[0]);
                close(output[1]);
            }
            return OK;
        }
    public:
        EndPoint(int sock)
            :_sock(sock)
        {}
        //读取请求
        void RecvHttpRequest()
        {
            RecvHttpRequestLine();
            RecvHttpRequestHeader();
            ParseHttpRequestLine();
            ParseHttpRequestHeader();
            RecvHttpRequestBody();
        }
        //构建响应
        void BuildHttpResponse()
        {
            auto& code = _http_response._status_code;
            std::string path;
            struct stat st;
            size_t pos = 0;
            if(_http_request._method != "GET"&&_http_request._method != "POST"){
                //非法请求
                LOG(WARNING, "method is not right");
                code = NOT_FOUND;
                goto END;
            }
            if(_http_request._method == "GET"){
                size_t pos = _http_request._uri.find('?');
                if(pos != std::string::npos){
                    Util::CutString(_http_request._uri, _http_request._path, _http_request._query_string, "?");
                    _http_request._cgi = true; //上传了参数，需要使用CGI模式
                }
                else{
                    _http_request._path = _http_request._uri;
                }
            }
            else if(_http_request._method == "POST"){
                _http_request._cgi = true; //上传了参数，需要使用CGI模式
            }
            else{
                //Do Nothing
            }
            path = _http_request._path;
            _http_request._path = WEB_ROOT;
            _http_request._path += path;

            if(_http_request._path[_http_request._path.size() - 1] == '/'){
                _http_request._path += HOME_PAGE;
            }
            
            if(stat(_http_request._path.c_str(), &st) == 0){
                //资源存在
                if(S_ISDIR(st.st_mode)){ //是一个目录，并且不会以/结尾，因为前面已经处理过了
                    _http_request._path += "/";
                    _http_request._path += HOME_PAGE;
                    stat(_http_request._path.c_str(), &st); //path改变，需要重新获取属性
                }
                else if(st.st_mode&S_IXUSR||st.st_mode&S_IXGRP||st.st_mode&S_IXOTH){ //是一个可执行程序，需要特殊处理
                    _http_request._cgi = true; //请求的是一个可执行程序，需要使用CGI模式
                }
                _http_response._size = st.st_size;
            }
            else{
                //资源不存在
                LOG(WARNING, _http_request._path + " NOT_FOUND");
                code = NOT_FOUND;
                goto END;
            }

            pos = _http_request._path.rfind('.');
            if(pos == std::string::npos){
                _http_response._suffix = ".html"; //默认设置
            }
            else{
                _http_response._suffix = _http_request._path.substr(pos);
            }

            if(_http_request._cgi == true){
                code = ProcessCgi(); //以CGI的方式进行处理
            }
            else{
                code = ProcessNonCgi(); //简单的网页返回，返回静态网页
            }

END:
            if(code != OK){

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
            sendfile(_sock, _http_response._fd, nullptr, _http_response._size);
            //关闭文件
            close(_http_response._fd);
        }
        ~EndPoint()
        {}
};

class Entrance{
    public:
        static void* HandlerRequest(void* arg)
        {
            LOG(INFO, "handler request begin");
            int sock = *(int*)arg;
            delete (int*)arg;
            std::cout<<"get a new link..."<<sock<<std::endl;

#ifdef DEBUG
            char buffer[4096];
            recv(sock, buffer, sizeof(buffer), 0);
            std::cout<<"------------------begin------------------"<<std::endl;
            std::cout<<buffer<<std::endl;
            std::cout<<"-------------------end-------------------"<<std::endl;
#else
            EndPoint* ep = new EndPoint(sock);
            ep->RecvHttpRequest();
            ep->BuildHttpResponse();
            ep->SendHttpResponse();

            close(sock);
            delete ep;
#endif
            LOG(INFO, "handler request end");
            return nullptr;
        }
};
