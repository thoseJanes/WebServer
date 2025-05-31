#ifndef WEBSERVER_HTTP_HTTPPARSER_H
#define WEBSERVER_HTTP_HTTPPARSER_H
#include "../net/connBuffer.h"
#include "../time/timeStamp.h"
#include "httpRequest.h"
#include <memory>
namespace webserver{

class HttpParser{
public:
    enum ParserState{
        sExpectRequestLine,
        sExpectRequestHeader,
        sExpectRequestBody,
        sGotAll
    };
    HttpParser();
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
    bool parseRequest(ConnBuffer* buf, TimeStamp time){
        bool hasMore = buf->readableBytes() > 0;
        ParserState state = sExpectRequestLine;
        if(request_){
            request_.reset();
        }
        
        while(hasMore){
            if(state == sExpectRequestLine){
                const char* end = buf->findCRLF(buf->readerBegin());
                if(end==NULL){
                    hasMore = false;
                }else if(parseRequestLine(buf->begin(), end)){
                    state = sExpectRequestHeader;
                    buf->retrieveTo(end+2);
                }else{
                    hasMore = false;
                }
            }else if(state == sExpectRequestHeader){
                const char* end = buf->findCRLF(buf->readerBegin());
                if(end==NULL){
                    hasMore = false;
                }else if(end == buf->readerBegin()){
                    state == sExpectRequestBody;
                    buf->retrieveTo(end+2);
                }else if(parseRequestHeader(buf->readerBegin(), end)){
                    buf->retrieveTo(end+2);
                }
            }else if(state == sExpectRequestBody){
                request_->setBody(string_view(buf->readerBegin(), buf->readableBytes()));
                buf->retrieveAll();
                state = sGotAll;
            }
        }
        return state == sGotAll;
    }
    const HttpRequest* getRequest(){
        return request_.get();
    }
    HttpRequest resetRequest(){
        request_.reset();
    }
private:
    unique_ptr<HttpRequest> request_;
};


}
#endif