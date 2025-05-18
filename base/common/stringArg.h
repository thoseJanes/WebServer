#ifndef WEBSERVER_COMMON_STRINGARG_H
#define WEBSERVER_COMMON_STRINGARG_H

#include <string>


namespace webserver{
using namespace std;
class StringArg{
public:
    StringArg(string str):str_(str.c_str()){}
    StringArg(const char* str):str_(str){}
    const char* c_str() const {return str_;}
private:
    const char* str_;
};

}

#endif