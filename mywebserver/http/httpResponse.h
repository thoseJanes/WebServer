#ifndef MYWEBSERVER_HTTP_HTTPRESPONSE_H
#define MYWEBSERVER_HTTP_HTTPRESPONSE_H
#include <string>
#include <map>
#include <assert.h>
#include <sys/stat.h>
#include "httpRequest.h"
#include "../../mybase/logging/logger.h"
using namespace std;



namespace mywebserver
{

namespace http{
    extern const map<int, string> StatusCodeToExplain;
}

//应当是：通过request来构造response（以获取version）？
class HttpResponse:public http::HttpMessage{
public:
    typedef http::Version Version;
    typedef http::BodyType BodyType;
    HttpResponse(const HttpRequest* request):statusCode_(-1), contentType_(kData){
        version_ = request->getVersion();
    }
    HttpResponse():statusCode_(-1){
        version_ = Version::vUNKNOW;
    }
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

    void setBody(string_view str) override {
        if(!body_.empty()){
            LOG_ERROR << "Failed in setBody. Body has been set data, please clear body content first.";
        }else{
            body_ = str;
        }
    }
    void appendBody(string_view str) override {
        if(kPath == contentType_){
            LOG_ERROR << "Failed in appendBody. Body has been set as file, please clear body content first.";
        }else{
            body_.append(str);
        }
    }
    void setBodyWithFile(string_view path){
        if(body_.empty()){
            contentType_ = kPath;
            body_ = path;
        }else{
            LOG_ERROR << "Failed in setBodyWithFile. Body has been set data, please clear body content first.";
        }
    }

    BodyType getBodyType() const override {
        if(header_.find("Transfer-Encoding") != header_.end() && getHeaderValue("Transfer-Encoding") == "chunked"){
            return BodyType::bTransferEncoding;
        }else if(header_.find("Content-Length") != header_.end()){
            return BodyType::bContentLength;
        }else{
            return BodyType::bNoBody;
        }
    }

    bool trySetBodyWithFile(const string& path){
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

    int getStatusCode() const {
        return statusCode_;
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

    void redirectTo(const string& path){
        this->setStatusCode(302);
        this->setHeaderValue("Location", path);
    }
private:
    void formatWithoutBody(string& out){
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", statusCode_);

        
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
    string body_;//body_的两种设置方式会增加复杂性，对于读文件考虑直接用另外的path_表示。
    // string path_;

    enum ContentType{
        kData,
        kPath
    };
    ContentType contentType_;
};

} // namespace mywebserver

#endif