// #include "../http/httpRequest.h";
// #include "../../mybase/logging/logger.h"

#include <map>
#include <string>
#include <algorithm>
#include <assert.h>
using namespace std;
map<string, string> urlToPath{
    {"/", "/desktop.html"},
    // {"/desktop.css", "/desktop.css"},
    {"/favicon.ico", "/favicon32.png"},
    {"/login", "/register.html"}
};

map<string, string> suffixToContentType{
    {"html", "text/html"},
    {"css", "text/css"},
    {"png", "image/png"},
    {"js", "application/x-javascript"},
    {"", "application/octet-stream"}
};
string getContentType(string& path){
    string dir = urlToPath[path];
    auto dotPos = dir.rfind(".");
    if(dotPos != std::string::npos){
        char buf[16];
        auto endPos = std::transform(dir.begin()+dotPos+1, dir.end(), buf, ::tolower);
        *endPos = '\0';
        if(suffixToContentType.find(buf) != suffixToContentType.end()){
            return suffixToContentType.at(buf);
        }
    }
    return suffixToContentType.at("");
}

string_view trim(const char* start, const char* end){
    while(end > start && ' ' == *(end-1)){
        end --;
    }
    while(' ' == *start && start < end){
        start ++;
    }
    ssize_t len = end - start;
    printf("%ld\n", len);
    assert(len >= 0);
    return string_view(start, len);
}

std::map<string, string> resolveContentPair(string_view content, char splitChar){
    std::map<string, string> pairMap;
    bool hasMore = true;
    size_t present = 0;
    size_t next;
    size_t middle;
    while(hasMore){
        middle = content.find('=', present);
        if(middle == string_view::npos){
            hasMore = false;
        }else{
            next = content.find(splitChar, present);
            if(next == string_view::npos){
                next = content.size();
                hasMore = false;
            }
            
            const char* data = content.data();
            pairMap.insert({string(trim(data+present, data+middle)), string(trim(data+middle+1, data+next))});
            present = next + 1;
        }
    }
    return pairMap;
}


int main(){
    auto test = resolveContentPair("   desktop   =   ksd  ", ';');

    printf("-%s,%s-\n", test.begin()->first.c_str(), test.begin()->second.c_str());

    string path = "/favicon.ico";
    //LOG_INFO << getContentType(path);
    string pair = "a=1&b=2&c=3&d=4";
    //auto map = webserver::http::resolveContentPair(pair.c_str());
    //LOG_INFO << map.at("a") << map.at("b") << map.at("d");

    char buf[256] = "SELECT username, password FROM users WHERE username='";
    //strcat(buf, "';");
    //LOG_INFO << "Sql query: "<<buf;
}
