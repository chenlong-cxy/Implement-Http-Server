#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>

//获取参数
bool GetQueryString(std::string& query_string)
{
    bool result = false;
    std::string method = getenv("METHOD"); //获取请求方法
    if(method == "GET"){ //GET方法通过环境变量获取参数
        query_string = getenv("QUERY_STRING");
        result = true;
    }
    else if(method == "POST"){ //POST方法通过管道获取参数
        int content_length = atoi(getenv("CONTENT_LENGTH"));
        //从管道中读取content_length个参数
        char ch = 0;
        while(content_length){
            read(0, &ch, 1);
            query_string += ch;
            content_length--;
        }
        result = true;
    }
    else{
        //Do Nothing
        result = false;
    }
    return result;
}
//切割字符串
bool CutString(std::string& in, const std::string& sep, std::string& out1, std::string& out2)
{
    size_t pos = in.find(sep);
    if(pos != std::string::npos){
        out1 = in.substr(0, pos);
        out2 = in.substr(pos + sep.size());
        return true;
    }
    return false;
}
int main()
{
    std::string query_string;
    GetQueryString(query_string); //获取参数

    //以&为分隔符将两个操作数分开
    std::string str1;
    std::string str2;
    CutString(query_string, "&", str1, str2);

    //以=为分隔符分别获取两个操作数的值
    std::string name1;
    std::string value1;
    CutString(str1, "=", name1, value1);
    std::string name2;
    std::string value2;
    CutString(str2, "=", name2, value2);

    //处理数据
    int x = atoi(value1.c_str());
    int y = atoi(value2.c_str());
    std::cout<<"<html>";
    std::cout<<"<head><meta charset=\"UTF-8\"></head>";
    std::cout<<"<body>";
    std::cout<<"<h3>"<<x<<" + "<<y<<" = "<<x+y<<"</h3>";
    std::cout<<"<h3>"<<x<<" - "<<y<<" = "<<x-y<<"</h3>";
    std::cout<<"<h3>"<<x<<" * "<<y<<" = "<<x*y<<"</h3>";
    std::cout<<"<h3>"<<x<<" / "<<y<<" = "<<x/y<<"</h3>"; //除0后cgi程序崩溃，属于异常退出
    std::cout<<"</body>";
    std::cout<<"</html>";

    return 0;
}

