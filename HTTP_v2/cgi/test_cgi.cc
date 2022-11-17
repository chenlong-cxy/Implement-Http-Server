#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>

bool GetQueryString(std::string& query_string)
{
    bool result = false;
    std::string method = getenv("METHOD");
    //std::cerr<<"debug METHOD: "<<method<<std::endl;
    if(method == "GET"){
        query_string = getenv("QUERY_STRING");
        //std::cerr<<"GET debug QUERY_STRING: "<<query_string<<std::endl;
        result = true;
    }
    else if(method == "POST"){
        int content_length = atoi(getenv("CONTENT_LENGTH"));
        //std::cerr<<"debug CONTENT_LENGTH: "<<content_length<<std::endl;
        char ch = 0;
        while(content_length){
            read(0, &ch, 1);
            query_string += ch;
            content_length--;
        }
        //std::cerr<<"POST debug QUERY_STRING: "<<query_string<<std::endl;
        result = true;
    }
    else{
        //Do Nothing
        result = false;
    }
    return result;
}
void CutString(std::string& in, const std::string& sep, std::string& out1, std::string& out2)
{
    size_t pos = in.find(sep);
    if(pos != std::string::npos){
        out1 = in.substr(0, pos);
        out2 = in.substr(pos + sep.size());
    }
}
int main()
{
    std::string query_string;
    GetQueryString(query_string);

    std::string str1;
    std::string str2;
    CutString(query_string, "&", str1, str2);

    std::string name1;
    std::string value1;
    CutString(str1, "=", name1, value1);
    std::string name2;
    std::string value2;
    CutString(str2, "=", name2, value2);

    std::cerr<<name1<<":"<<value1<<std::endl;
    std::cerr<<name2<<":"<<value2<<std::endl;

    std::cout<<name1<<":"<<value1<<std::endl;
    std::cout<<name2<<":"<<value2<<std::endl;

    //处理数据
    int x = atoi(value1.c_str());
    int y = atoi(value2.c_str());
    std::cout<<"<html>";
    std::cout<<"<head><meta charset=\"UTF-8\"></head>";
    std::cout<<"<body>";
    std::cout<<"<h3>"<<x<<"+"<<y<<"="<<x+y<<"</h3>";
    std::cout<<"<h3>"<<x<<"-"<<y<<"="<<x-y<<"</h3>";
    std::cout<<"<h3>"<<x<<"*"<<y<<"="<<x*y<<"</h3>";
    std::cout<<"<h3>"<<x<<"/"<<y<<"="<<x/y<<"</h3>"; //除0后cgi程序崩溃，属于正常退出
    std::cout<<"</body>";
    std::cout<<"</html>";
    return 0;
}

//#include <iostream>
//#include <cstdlib>
//
//int main()
//{
//    std::cerr<<"CGI Process Run..."<<std::endl;
//    //std::string test_string;
//    //std::cin>>test_string;
//    //std::cerr<<test_string<<std::endl;
//    std::cerr<<getenv("METHOD")<<std::endl;
//    std::cerr<<getenv("QUERY_STRING")<<std::endl;
//    return 0;
//}
