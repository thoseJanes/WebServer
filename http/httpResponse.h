#ifndef WEBSERVER_HTTP_HTTPRESPONSE_H
#define WEBSERVER_HTTP_HTTPRESPONSE_H
#include <string>
#include <map>
#include <assert.h>
#include "httpRequest.h"
using namespace std;



namespace webserver
{

namespace http{
    extern const map<int, string> StatusCodeToExplain;
}

class HttpResponse{
public:
    typedef http::Version Version;
    HttpResponse(){}
    ~HttpResponse(){}
    string toString(){
        string out;
        out += http::versionToString(version_) + " ";
        char buf[8];
        int len = snprintf(buf, sizeof(buf), "%d", statusCode_);
        out += buf;
        out += " ";
        out += http::StatusCodeToExplain.at(statusCode_);
        out += "\r\n";
        for(auto it = header_.begin();it!=header_.end();it++){
            out += it->first + ":" + it->second + "\r\n";
        }
        out += "\r\n";
        out += body_;
        return out;
    }

    const string& getHeaderValue(string& item) const {
        if(header_.find(item) == header_.end()){
            return "";
        }
        return header_.at(item);
    }
    const Version& getVersion(){
        return version_;
    }
    const string& getBody(){
        return body_;
    }
    
    const string& setHeaderValue(string item, string value){
        if(header_.find(item) == header_.end()){
            header_.insert({item, value});
        }else{
            header_.at(item) = value;
        }
    }
    void setVersion(Version version){
        version_ = version;
    }
    void setBody(string_view str){
        body_ = str;
    }
    
private:
    int statusCode_;
    Version version_;
    map<string, string> header_;
    string body_;
};

} // namespace webserver

#endif