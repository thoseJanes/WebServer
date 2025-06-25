#ifndef MYWEBSERVER_HTTP_HTTPPARSER_H
#define MYWEBSERVER_HTTP_HTTPPARSER_H
#include "../../myNetLib/net/connBuffer.h"
#include "../../myNetLib/time/timeStamp.h"
#include "httpRequest.h"

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
        sExpectRequestLine,
        sExpectRequestHeader,
        sExpectConetentBody,
            sExpectBlockBodySize,
            sExpectBlockBodyData,
            sExpectBlockBodyEnd,
        sGotAll
    };
    HttpParser():state_(sExpectRequestLine){}
    ~HttpParser() = default;

    bool parseRequestLine(const char* start, const char* end);
    bool parseRequestHeader(const char* start, const char* end);
    ParserState freshBodyTypeStatus();

    bool parseRequest(ConnBuffer* buf, TimeStamp time);

    const HttpRequest* getRequest(){
        return request_.get();
    }
    void reset(){
        // HttpRequest dummy;
        // request_.swap(dummy);
        state_ = sExpectRequestLine;
        request_.reset();
    }
    ParserState getState() const {
        return state_;
    }
private:
    typedef http::Version Version;
    typedef http::Method Method;
    typedef http::BodyType BodyType;

    unique_ptr<HttpRequest> request_;
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
