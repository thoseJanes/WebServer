#ifndef WEBSERVER_HTTP_HTTPRESPONSE_H
#define WEBSERVER_HTTP_HTTPRESPONSE_H
#include <string>
#include <map>
#include <assert.h>
#include "httpRequest.h"
#include "../logging/logger.h"
using namespace std;



namespace webserver
{

namespace http{
    extern const map<int, string> StatusCodeToExplain;
}

//应当是：通过request来构造response（以获取version）？
class HttpResponse{
public:
    typedef http::Version Version;
    HttpResponse(const HttpRequest* request){version_ = request->getVersion();}
    ~HttpResponse(){}
    string toStringWithoutBody(){
        string out;
        char buf[8];
        int len = snprintf(buf, sizeof(buf), "%d", statusCode_);
        //检查是否存在长度信息。
        if(header_.find("Transfer-Encoding") != header_.end()){
            LOG_ERROR << "Transfer-Encoding is not supported";
            header_.erase("Transfer-Encoding");
        }
        auto lengthItem = header_.find("Content-Length");
        if(lengthItem != header_.end()){
            size_t contentLength = static_cast<size_t>(atol(lengthItem->second.c_str()));
            if(body_.size() != contentLength){
                LOG_ERROR << "Content-Length not equals body size!";
            }
        }else if(lengthItem == header_.end()){
            header_.insert({"Content-Length", to_string(body_.size())});
        }

        out.append(http::versionToString(version_));
        out.push_back(' ');
        out.append(buf);
        out.push_back(' ');
        out.append(http::StatusCodeToExplain.at(statusCode_));
        out += "\r\n";
        for(auto it = header_.begin();it!=header_.end();it++){
            out.append(it->first);
            out.push_back(':');
            out.append(it->second);
            out += "\r\n";
        }
        out += "\r\n";
        return out;
    }
    string toString(){
        string out;
        char buf[8];
        int len = snprintf(buf, sizeof(buf), "%d", statusCode_);
        //检查是否存在长度信息。
        if(header_.find("Transfer-Encoding") != header_.end()){
            LOG_ERROR << "Transfer-Encoding is not supported";
            header_.erase("Transfer-Encoding");
        }
        auto lengthItem = header_.find("Content-Length");
        if(lengthItem != header_.end()){
            size_t contentLength = static_cast<size_t>(atol(lengthItem->second.c_str()));
            if(body_.size() != contentLength){
                LOG_ERROR << "Content-Length not equals body size!";
            }
        }else if(lengthItem == header_.end()){
            header_.insert({"Content-Length", to_string(body_.size())});
        }

        out.append(http::versionToString(version_));
        out.push_back(' ');
        out.append(buf);
        out.push_back(' ');
        out.append(http::StatusCodeToExplain.at(statusCode_));
        out += "\r\n";
        for(auto it = header_.begin();it!=header_.end();it++){
            out.append(it->first);
            out.push_back(':');
            out.append(it->second);
            out += "\r\n";
        }
        out += "\r\n";
        out.append(body_);
        return out;
    }

    string getHeaderValue(string& item) const {
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

    void setHeaderValue(string item, string value){
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
    void appendBody(string_view str){
        body_.append(str);
    }
    void setStatusCode(int statusCode){
        statusCode_ = statusCode;
    }

private:
    int statusCode_;
    Version version_;
    map<string, string> header_;
    string body_;
};

} // namespace webserver

#endif