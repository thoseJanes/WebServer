#include "httpRequest.h"
#include "../logging/logger.h"
#include <string.h>

namespace webserver{

string http::versionToString(Version version){
    switch (version)
    {
    case vHTTP1_0:
        return "HTTP/1.0";
    case vHTTP1_1:
        return "HTTP/1.1";
    case vUNKNOW:
        return "";
    default:
        return "";
    }
}

string http::methodToString(Method method){
    #define METHOD_TO_STRING_GENERATOR(method_) \
        else if(m ## method_ == method){return #method_;}

    if(mUNKNOW == method){return "";}
    METHOD_TO_STRING_GENERATOR(GET)
    METHOD_TO_STRING_GENERATOR(POST)
    METHOD_TO_STRING_GENERATOR(HEAD)
    METHOD_TO_STRING_GENERATOR(OPTIONS)
    METHOD_TO_STRING_GENERATOR(PUT)
    METHOD_TO_STRING_GENERATOR(DELETE)
    METHOD_TO_STRING_GENERATOR(TRACE)
    else{LOG_FATAL << "Failed in methodToString(). Method is wrong.";abort();}
    #undef METHOD_TO_STRING_GENERATOR
}

//end指最后一个字符的下一个字符。
string_view http::trim(const char* start, const char* end){
    while(end > start && ' ' == *(end-1)){
        end --;
    }
    while(' ' == *start && start < end){
        start ++;
    }
    ssize_t len = end - start;
    assert(len >= 0);
    return string_view(start, len);
}

std::map<string, string> http::resolveContentPair(string_view content, char splitChar){
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

namespace http{
const map<string, string> suffixToContentType = {
    {"html", "text/html"},
    {"css", "text/css"},
    {"png", "image/png"},
    {"js", "application/x-javascript"},
    {"jpg", "image/jpeg"},
    {"", "application/octet-stream"}
};
}


string http::getContentType(string_view dir){
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

}


