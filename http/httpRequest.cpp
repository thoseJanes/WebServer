#include "httpRequest.h"

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
    else{assert(false);}
    #undef METHOD_TO_STRING_GENERATOR
}


}


