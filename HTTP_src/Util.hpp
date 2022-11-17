#pragma once

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>

//工具类
class Util{
    public:
        static int ReadLine(int sock, std::string& out)
        {
            char ch = 'X'; //随便设置一个字符，只要不是\n即可
            while(ch != '\n'){
                ssize_t size = recv(sock, &ch, 1, 0);
                if(size > 0){
                    if(ch == '\r'){
                        //窥探
                        recv(sock, &ch, 1, MSG_PEEK);
                        if(ch == '\n'){
                            //窥探成功
                            //\r\n->\n
                            recv(sock, &ch, 1, 0);
                        }
                        else{
                            //\r->\n
                            ch = '\n';
                        }
                    }
                    //普通字符或\n
                    out.push_back(ch);
                }
                else if(size == 0){
                    return 0;
                }
                else{
                    return -1;
                }
            }
            return out.size();
        }
        static bool CutString(std::string& target, std::string& sub1_out, std::string& sub2_out, std::string sep)
        {
            size_t pos = target.find(sep, 0);
            if(pos != std::string::npos){
                sub1_out = target.substr(0, pos);
                sub2_out = target.substr(pos + sep.size());
                return true;
            }
            return false;
        }
};
