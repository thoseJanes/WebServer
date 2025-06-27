#ifndef MYWEBSERVER_HTTP_HTTPPARSER_H
#define MYWEBSERVER_HTTP_HTTPPARSER_H
#include "../../mynetlib/net/connBuffer.h"
#include "../../mybase/time/timeStamp.h"
#include "httpRequest.h"
#include "httpResponse.h"

#include <memory>

using namespace mynetlib;
namespace mywebserver{

class HttpParser:Noncopyable{
public:
    /*
        body部分有4种情况：
        1、头部存在Content-Length，则body长度为该选项对应值。
        2、头部存在Transfer-Encoding: chunked，则body由块决定，具体来说：
            每个块的格式为<length><CRLF><data><CRLF>
            当块结束时，会有一个长度为0的块来标识，即0\r\n\r\n
        3、以上两个选项都不存在，则在对端停止发送前皆为body。（Http 1.0，不可靠)

        4、有些方法没有body，也不会给出Content-Length，比如get！

        当Content-Length和Transfer-Encoding同时出现时，优先处理Transfer-Encoding
    */
    enum ParserState{
        sExpectLine,
        sExpectHeader,
        sExpectConetentBody,
            sExpectBlockBodySize,
            sExpectBlockBodyData,
            sExpectBlockBodyEnd,
        sGotAll
    };
    enum MessageType{
        tRequest,
        tResponse,
        tUnknow
    };
    HttpParser():state_(sExpectLine), messageType_(tUnknow){}
    ~HttpParser() = default;

    bool parseRequest(ConnBuffer* buf, TimeStamp time);
    bool parseResponse(ConnBuffer* buf);

    const HttpRequest* getRequest(){
        assert(messageType_ == tRequest);
        return dynamic_cast<HttpRequest*>(message_.get());
    }
    const HttpResponse* getResponse(){
        assert(messageType_ == tResponse);
        return dynamic_cast<HttpResponse*>(message_.get());
    }
    void reset(){
        // HttpRequest dummy;
        // request_.swap(dummy);
        state_ = sExpectLine;
        // request_.reset();
        // response_.reset();
        message_.reset();
    }
    ParserState getState() const {
        return state_;
    }
private:
    bool parseRequestLine(const char* start, const char* end);
    bool parseResponseLine(const char* start, const char* end);
    bool parseLine(const char* start, const char* end){
        if(tRequest == messageType_){
            return parseRequestLine(start, end);
        }else if(tResponse == messageType_){
            return parseResponseLine(start, end);
        }else{
            LOG_FATAL << "HttpParser::parseLine - Unknow message type!";
            return false;
        }
    }
    bool parseHeader(const char* start, const char* end);
    bool parseLoop(ConnBuffer* buf);
    ParserState freshBodyTypeStatus();

    typedef http::Version Version;
    typedef http::Method Method;
    typedef http::BodyType BodyType;

    unique_ptr<http::HttpMessage> message_;
    MessageType messageType_;
    //unique_ptr<HttpResponse> response_;
    ParserState state_;

    BodyType bodyType_;
    union {
        size_t remainingBodyLen_;//固定长度body
        size_t remainingBlockLen_;//分块传输body
    };
    size_t kMaxBodyLength_;//允许的最大body长度
};


}
#endif
