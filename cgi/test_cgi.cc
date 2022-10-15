#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>

int main()
{
    std::string method = getenv("METHOD");
    std::cerr<<"debug METHOD: "<<method<<std::endl;
    std::string query_string;
    if(method == "GET"){
        query_string = getenv("QUERY_STRING");
        std::cerr<<"GET debug QUERY_STRING: "<<query_string<<std::endl;
    }
    else if(method == "POST"){
        int content_length = atoi(getenv("CONTENT_LENGTH"));
        std::cerr<<"debug CONTENT_LENGTH: "<<content_length<<std::endl;
        char ch = 0;
        while(content_length){
            read(0, &ch, 1);
            query_string += ch;
            content_length--;
        }
        std::cerr<<"POST debug QUERY_STRING: "<<query_string<<std::endl;
    }
    else{
        //Do Nothing
    }

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
