#ifndef WEBSERVER_HTTP_HTTPRESPONSE_H
#define WEBSERVER_HTTP_HTTPRESPONSE_H
#include <string>
#include <map>
#include <assert.h>
#include <sys/stat.h>
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
    HttpResponse(const HttpRequest* request):version_(request->getVersion()), statusCode_(-1), contentType_(kData){}
    HttpResponse():version_(Version::vUNKNOW), statusCode_(-1){}
    ~HttpResponse(){}
    string toStringWithoutBody(){
        string out;
        // if(header_.find("Transfer-Encoding") != header_.end()){
        //     LOG_ERROR << "Transfer-Encoding is not supported";
        //     header_.erase("Transfer-Encoding");
        // }
        // auto lengthItem = header_.find("Content-Length");
        // if(lengthItem != header_.end()){
        //     // size_t contentLength = static_cast<size_t>(atol(lengthItem->second.c_str()));
        //     // if(body_.size() != contentLength){
        //     //     LOG_ERROR << "Content-Length not equals body size!";
        //     // }
        // }else if(lengthItem == header_.end()){
        //     header_.insert({"Content-Length", to_string(body_.size())});
        // }

        //不能随便加上Content-Length头部，因为body可能暂时还是个文件路径！
        formatWithoutBody(out);
        return out;
    }
    
    string toString(){
        string out;

        //如果文件较大，可以用分块传输的方式（不需要提前知道Content-Length）减少一次拷贝。
        if(contentType_ == kPath){
            AdaptiveFileReader fileContent(body_);
            clearBody();
            do{
                body_.append(fileContent.toStringView());
            }while(fileContent.mayHaveMore());
        }

        if(header_.find("Transfer-Encoding") != header_.end()){
            LOG_ERROR << "Transfer-Encoding is not supported";
            header_.erase("Transfer-Encoding");
        }
        auto lengthItem = header_.find("Content-Length");
        if(lengthItem != header_.end()){//如果已经加入了Content-length，是否需要比较真实长度？还是直接防止用户加上这个头？
            // size_t contentLength = static_cast<size_t>(atol(lengthItem->second.c_str()));
            // if(body_.size() != contentLength){
            //     LOG_ERROR << "Content-Length not equals body size!";
            // }
        }else if(lengthItem == header_.end()){
            header_.insert({"Content-Length", to_string(body_.size())});
        }
        //检查是否存在返回状态码
        formatWithoutBody(out);

        out.append(body_);
        
        return out;
    }
    string toStringFromBody(string_view body){
        if(isValid()){
            string out;
            header_.insert({"Content-Length", to_string(body.size())});
            formatWithoutBody(out);
            out.append(body);
            return out;
        }else{
            LOG_ERROR << "Failed in HttpResponse::toStringFromBody()";
            return "";
        }
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
        if(!body_.empty()){
            LOG_ERROR << "Failed in setBody. Body has been set data, please clear body content first.";
        }else{
            body_ = str;
        }
    }
    void appendBody(string_view str){
        if(contentType_ = kPath){
            LOG_ERROR << "Failed in appendBody. Body has been set as file, please clear body content first.";
        }else{
            body_.append(str);
        }
    }
    void setBodyWithFile(string path){
        if(body_.empty()){
            contentType_ = kPath;
            body_ = path;
        }else{
            LOG_ERROR << "Failed in setBodyWithFile. Body has been set data, please clear body content first.";
        }
        // SmallFileReader fileContent(path, 64*64*1024);
        //     appendBody(fileContent.toStringView());
    }
    bool trySetBodyWithFile(string path){
        if(detail::fileExists(path)){
            setBodyWithFile(path);
            return true;
        }
        LOG_ERROR << "Failed in trySetBodyWithFile. Path not exists";
        return false;
    }
    void clearBody(){
        body_ = "";
        contentType_ = kData;
    }
    

    void setStatusCode(int statusCode){
        statusCode_ = statusCode;
    }

    bool isValid(){
        if(version_ == Version::vUNKNOW){
            LOG_ERROR << "Failed in HttpResponse. Version is unknown";
        }else if(http::StatusCodeToExplain.find(statusCode_) == http::StatusCodeToExplain.end()){
            LOG_ERROR << "Failed in HttpResponse. Status code is invalid";
        }else{
            return true;
        }
        return false;
    }

    
private:
    void formatWithoutBody(string& out){
        char buf[8];
        int len = snprintf(buf, sizeof(buf), "%d", statusCode_);

        
        out.append(http::versionToString(version_));
        out.push_back(' ');
        out.append(buf);
        out.push_back(' ');
        assert(http::StatusCodeToExplain.find(statusCode_) != http::StatusCodeToExplain.end());
        out.append(http::StatusCodeToExplain.at(statusCode_));
        out += "\r\n";
        for(auto it = header_.begin();it!=header_.end();it++){
            out.append(it->first);
            out.push_back(':');
            out.append(it->second);
            out += "\r\n";
        }
        out += "\r\n";
    }
    
    int statusCode_;
    Version version_;
    map<string, string> header_;
    string body_;

    enum ContentType{
        kData,
        kPath
    };
    ContentType contentType_;
};

} // namespace webserver

#endif