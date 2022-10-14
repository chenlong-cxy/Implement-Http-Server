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
#include "Util.hpp"
#include "Log.hpp"

#define SEP ": "
#define WEB_ROOT "wwwroot"
#define HOME_PAGE "index.html"
//#define DEBUG 1

#define OK 200
#define NOT_FOUND 404

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
    public:
        HttpResponse()
            :_status_code(OK)
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

            LOG(INFO, _http_request._method);
            LOG(INFO, _http_request._uri);
            LOG(INFO, _http_request._version);
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
                //std::cout<<"debug: "<<key<<std::endl;
                //std::cout<<"debug: "<<value<<std::endl;
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
            return 0;
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
                }
                else if(st.st_mode&S_IXUSR||st.st_mode&S_IXGRP||st.st_mode&S_IXOTH){ //是一个可执行程序，需要特殊处理
                    _http_request._cgi = true; //请求的是一个可执行程序，需要使用CGI模式
                }
            }
            else{
                //资源不存在
                LOG(WARNING, _http_request._path + " NOT_FOUND");
                code = NOT_FOUND;
                goto END;
            }

            if(_http_request._cgi == true){
                //ProcessCgi(); //以CGI的方式进行处理
            }
            else{
                ProcessNonCgi(); //简单的网页返回，返回静态网页
            }

END:
            return;
        }
        //发送响应
        void SendHttpResponse()
        {}
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
