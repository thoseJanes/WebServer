#ifndef WEBSERVER_HTTP_HTTPREQUEST_H
#define WEBSERVER_HTTP_HTTPREQUEST_H
#include <string>
#include <string_view>
#include <map>
#include <assert.h>
#include "../time/timeStamp.h"
using namespace std;

namespace webserver
{
namespace http{
    enum Method{
        mGET,// 请求获取由Request-URI所标识的资源。
        mPOST,// 在Request-URI所标识的资源后附加新的数据。
        mHEAD,// 请求获取由Request-URI所标识的资源的响应消息报头。
        mOPTIONS,// 请求查询服务器的性能，或查询与资源相关的选项和需求。
        mPUT,// 请求服务器存储一个资源，并用Request-URI作为其标识。
        mDELETE,// 请求服务器删除由Request-URI所标识的资源。
        mTRACE,// 请求服务器回送收到的请求信息，主要用语测试或诊断。
        mUNKNOW
    };
    enum Version{
        vHTTP1_0,
        vHTTP1_1,
        vUNKNOW
    };
    enum BodyType{
        bTransferEncoding,
        bContentLength,
        bUntilClosed,
        bNoBody
    };

    string versionToString(Version version);

    string methodToString(Method method);

    map<string, string> resolveContentPair(string_view content, char splitChar);

    //const map<string, string> suffixToContentType;
    
    string getContentType(string& dir);

    string_view trim(const char* start, const char* end);

    // class HttpMessage{
    // public:
    //     typedef http::Version Version;
    //     typedef http::Method Method;
    //     friend HttpRequest;
    //     HttpMessage(){}
    //     ~HttpMessage(){}
    //     string getHeaderValue(string& item) const {
    //         if(header_.find(item) == header_.end()){
    //             return "";
    //         }
    //         return header_.at(item);
    //     }
    //     const Version& getVersion(){
    //         return version_;
    //     }
    //     const string& getBody(){
    //         return body_;
    //     }
        
    //     void setHeaderValue(string item, string value){
    //         if(header_.find(item) == header_.end()){
    //             header_.insert({item, value});
    //         }else{
    //             header_.at(item) = value;
    //         }
    //     }
    //     void setVersion(Version version){
    //         version_ = version;
    //     }
    //     void setBody(string_view str){
    //         body_ = str;
    //     }
    
    // private:
    //     Version version_;
    //     map<string, string> header_;
    //     string body_;
    // };
}


class HttpRequest{
public:
    typedef http::Version Version;
    typedef http::Method Method;
    typedef http::BodyType BodyType;
    HttpRequest(TimeStamp time):reqTime_(time), version_(http::vUNKNOW), method_(http::mUNKNOW){}
    ~HttpRequest(){}

    string getHeaderValue(string& item) const {
        if(header_.find(item) == header_.end()){
            return "";
        }
        return header_.at(item);
    }
    string getHeaderValue(string_view item) const {
        string itemString = string(item);
        if(header_.find(itemString) == header_.end()){
            return "";
        }
        return header_.at(itemString);
    }
    const Version& getVersion() const {
        return version_;
    }
    const string& getBody() const {
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

    const Method& getMethod() const {return method_;}
    void setMethod(Method method){
        method_ = method;
    }

    void resolveMethod(string_view str){
        #define RESOLVE_METHOD_GENERATOR(method) \
            else if(#method == str){setMethod(http::m ## method);}

        if(str.empty()){setMethod(http::mUNKNOW);}
        RESOLVE_METHOD_GENERATOR(GET)
        RESOLVE_METHOD_GENERATOR(POST)
        RESOLVE_METHOD_GENERATOR(HEAD)
        RESOLVE_METHOD_GENERATOR(OPTIONS)
        RESOLVE_METHOD_GENERATOR(PUT)
        RESOLVE_METHOD_GENERATOR(DELETE)
        RESOLVE_METHOD_GENERATOR(TRACE)
        else{setMethod(http::mUNKNOW);}
        #undef RESOLVE_METHOD_GENERATOR
    }
    void resolveVersion(string_view str){
        if("HTTP/1.0" == str){
            version_ = http::vHTTP1_0;
        }else if("HTTP/1.1" == str){
            version_ = http::vHTTP1_1;
        }else{
            version_ = http::vUNKNOW;
        }
    }
    void setPath(string_view str){
        path_ = str;
    }
    void setMessage(string_view str){
        message_ = str;
    }
    const string& getMessage() const {return message_;}
    const string& getPath() const {return path_;}

    bool valid() const {
        return version_!=http::vUNKNOW && method_ != http::mUNKNOW;
    }
    string toString() const {
        string out;
        if(!valid()){return "";}
        out += http::methodToString(method_);
        out += " ";
        out += path_;
        if(!message_.empty()){
            out += "?";
            out += message_;
        }
        out += " ";
        out += http::versionToString(version_);
        out += "\r\n";
        for(auto it = header_.begin();it!=header_.end();it++){
            out += it->first + ":" + it->second + "\r\n";
        }
        out += "\r\n";
        out += body_;
        return out;
    }
    TimeStamp getRequestTime() const {
        return reqTime_;
    }

    //当Content-Length和Transfer-Encoding同时出现时，优先处理Transfer-Encoding
    BodyType getBodyType() const {
        if(method_ == Method::mGET){
            return BodyType::bNoBody;
        }else if(header_.find("Transfer-Encoding") != header_.end() && getHeaderValue("Transfer-Encoding") == "chunked"){
            return BodyType::bTransferEncoding;
        }else if(header_.find("Content-Length") != header_.end()){
            return BodyType::bContentLength;
        }else{
            return BodyType::bUntilClosed;
        }
    }

    void swap(HttpRequest& that){
        std::swap(method_, that.method_);
        std::swap(version_, that.version_);
        path_.swap(that.path_);
        message_.swap(that.message_);
        header_.swap(that.header_);
        body_.swap(that.body_);
        reqTime_.swap(that.reqTime_);
    }

private:
    
    Method method_;
    string path_;
    string message_;
    
    TimeStamp reqTime_;
    Version version_;
    map<string, string> header_;
    
    string body_;

    
    //const void* request_;
};




} // namespace webserver

#endif