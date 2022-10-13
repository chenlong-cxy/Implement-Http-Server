#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "Util.hpp"
#include "Log.hpp"

#define SEP ": "

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
    public:
        HttpRequest()
            :_content_length(0)
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
        {}
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
