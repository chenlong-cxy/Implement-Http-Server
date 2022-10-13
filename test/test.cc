#include <iostream>
#include <string>
#include <sstream>

int main()
{
    std::string request_line = "GET /s/index.html HTTP/1.1";
    std::stringstream ss(request_line);
    //ss<<request_line;
    std::string method;
    std::string uri;
    std::string version;
    ss>>method>>uri>>version;
    std::cout<<"method: "<<method<<std::endl;
    std::cout<<"uri: "<<uri<<std::endl;
    std::cout<<"version: "<<version<<std::endl;
    return 0;
}

//#include <iostream>
//#include <ctime>
//
//int main()
//{
//    std::cout<<time(nullptr)<<std::endl;
//    std::cout<<__FILE__<<std::endl;
//    std::cout<<__LINE__<<std::endl;
//    return 0;
//}
