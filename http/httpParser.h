#ifndef WEBSERVER_HTTP_HTTPPARSER_H
#define WEBSERVER_HTTP_HTTPPARSER_H
#include "../net/connBuffer.h"
#include "../time/timeStamp.h"
#include "httpRequest.h"
#include <memory>
namespace webserver{

class HttpParser{
public:
    /*
        body部分有三种情况：
        1、头部存在Content-Length，则body长度为该选项对应值。
        2、头部存在Transfer-Encoding: chunked，则body由块决定，具体来说：
            每个块的格式为<length><CRLF><data><CRLF>
            当块结束时，会有一个长度为0的块来标识，即0\r\n\r\n
        3、以上两个选项都不存在，则在对端停止发送前皆为body。（Http 1.0，不可靠)

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
    ~HttpParser();

    bool parseRequestLine(const char* start, const char* end){
        const char* mEnd = std::find(start, end, " ");
        if(mEnd == end){
            return false;
        }
        request_->resolveMethod(string_view(start, mEnd-start));
        mEnd++;

        const char* uEnd = std::find(mEnd, end, " ");
        if(uEnd == end){
            return false;
        }
        const char* meStart = std::find(mEnd, uEnd, "?");
        request_->setPath(string_view(mEnd, meStart-mEnd));
        if(meStart != uEnd){
            meStart++;
            request_->setMessage(string_view(meStart, uEnd-meStart));
        }
        uEnd++;

        request_->resolveVersion(string_view(uEnd, end-uEnd));

        return request_->valid();
    }
    bool parseRequestHeader(const char* start, const char* end){
        while(*end == ' '){
            end--;
        }
        end++;
        const char* valueStart = std::find(start, end, ":");
        const char* itemStart = start;
        const char* itemEnd = valueStart-1;
        while(*itemEnd == ' '){
            itemEnd --;
        }
        itemEnd ++;
        valueStart++;
        size_t itemLen = static_cast<size_t>(itemEnd-itemStart);
        size_t valueLen = static_cast<size_t>(end-valueStart);
        if(itemLen){
            request_->setHeaderValue(string(itemStart, itemLen), string(valueStart, valueLen));
            return true;
        }else{
            return false;
        }
        
    }
    ParserState freshBodyTypeStatus(){
        bodyType_ = request_->getBodyType();
        if(bodyType_ == BodyType::bContentLength){
            remainingBlockLen_ = static_cast<size_t>(atol(request_->getHeaderValue("Content-Length").c_str()));
            if(remainingBlockLen_ > 0){
                return sExpectConetentBody;
            }else{
                return sGotAll;
            }
        }else if(bodyType_ == BodyType::bTransferEncoding){
            return sExpectBlockBodySize;
        }else if(bodyType_ == BodyType::bUntilClosed){
            return sExpectConetentBody;
        }else{
            LOG_FATAL << "Unknow body type";
        }
    }

    bool parseRequest(ConnBuffer* buf, TimeStamp time){
        bool hasMore = buf->readableBytes() > 0;
        bool valid = true;
        if(!request_){
            request_.reset(new HttpRequest(time));
        }

        while(hasMore){
            if(state_ == sExpectRequestLine){
                const char* end = buf->findCRLF(buf->readerBegin());
                if(end==NULL){
                    hasMore = false;
                }else if(parseRequestLine(buf->begin(), end)){
                    state_ = sExpectRequestHeader;
                    buf->retrieveTo(end+2);
                }else{
                    hasMore = false;
                    valid = false;
                }
            }else if(state_ == sExpectRequestHeader){
                const char* end = buf->findCRLF(buf->readerBegin());
                if(end==NULL){
                    hasMore = false;
                }else if(end == buf->readerBegin()){
                    state_ = freshBodyTypeStatus();
                    buf->retrieveTo(end+2);
                }else if(parseRequestHeader(buf->readerBegin(), end)){
                    buf->retrieveTo(end+2);
                }else{
                    hasMore = false;
                    valid = false;
                }
            }else if(state_ == sExpectConetentBody){
                if(bodyType_ == BodyType::bContentLength){
                    size_t getLen = min(buf->readableBytes(), remainingBodyLen_);
                    request_->appendBody(string_view(buf->readerBegin(), getLen));
                    buf->retrieve(getLen);
                    remainingBodyLen_ -= getLen;
                    state_ = (remainingBodyLen_==0)?sGotAll:sExpectConetentBody;
                    hasMore = (remainingBodyLen_==0)?false:true;
                }else if(bodyType_ == BodyType::bUntilClosed){
                    size_t getLen = buf->readableBytes();
                    request_->appendBody(string_view(buf->readerBegin(), getLen));
                    buf->retrieve(getLen);
                }
            }else if(state_ == sExpectBlockBodySize){
                const char* end = buf->findCRLF(buf->readerBegin());
                if(end==NULL){
                    hasMore = false;
                }else{
                    string blockLenString = string(buf->readerBegin(), end - buf->readerBegin());
                    remainingBlockLen_ = static_cast<size_t>(atol(blockLenString.c_str()));
                    buf->retrieveTo(end+2);//即使remainingBlockLen_为0,当前不一定能接收到4个crlf。怎么标识？
                    state_ = (remainingBlockLen_==0)?sExpectBlockBodyEnd:sExpectBlockBodyData;
                }
            }else if(state_ == sExpectBlockBodyData){
                if(remainingBlockLen_ == 0){
                    if(buf->readableBytes() >= 2){
                        auto crlf = buf->findCRLF(buf->readerBegin());
                        if(crlf == buf->readerBegin()){
                            buf->retrieve(2);
                            state_ = sExpectBlockBodySize;
                        }else{
                            hasMore = false;
                            valid = false;
                        }
                    }else{
                        hasMore = false;
                    }
                }else{
                    size_t getLen = min(buf->readableBytes(), remainingBlockLen_);
                    request_->appendBody(string_view(buf->readerBegin(), getLen));
                    buf->retrieve(getLen);
                    remainingBlockLen_ -= getLen;
                    state_ = sExpectBlockBodyData;
                }
            }else if(state_ == sExpectBlockBodyEnd){
                if(buf->readableBytes() >= 2){
                    auto crlf = buf->findCRLF(buf->readerBegin());
                    if(crlf == buf->readerBegin()){
                        buf->retrieve(2);
                        state_ = sGotAll;
                    }else{
                        hasMore = false;
                        valid = false;
                    }
                }else{
                    hasMore = false;
                }
            }else if(state_ == sGotAll){
                hasMore = false;
            }
        }
        return valid;
    }
    const HttpRequest* getRequest(){
        return request_.get();
    }
    HttpRequest reset(){
        request_.reset();
        state_ = sExpectRequestLine;
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