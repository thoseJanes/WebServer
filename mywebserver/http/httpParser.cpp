#include "httpParser.h"
#include "httpRequest.h"
#include "httpResponse.h"
#include <string>

using namespace mywebserver;

bool HttpParser::parseRequestLine(const char* start, const char* end){
    HttpRequest* request = dynamic_cast<HttpRequest*>(message_.get());
    const char* mEnd = std::find(start, end, ' ');
    if(mEnd == end){
        return false;
    }
    request->resolveMethod(string_view(start, mEnd-start));
    mEnd++;

    const char* uEnd = std::find(mEnd, end, ' ');
    if(uEnd == end){
        return false;
    }
    const char* meStart = std::find(mEnd, uEnd, '?');
    request->setPath(string_view(mEnd, meStart-mEnd));
    if(meStart != uEnd){
        meStart++;
        request->setMessage(string_view(meStart, uEnd-meStart));
    }
    uEnd++;

    request->resolveVersion(string_view(uEnd, end-uEnd));

    return request->valid();
}
bool HttpParser::parseResponseLine(const char* start, const char* end){
    HttpResponse* response = dynamic_cast<HttpResponse*>(message_.get());
    const char* vEnd = std::find(start, end, ' ');
    if(vEnd == end){
        return false;
    }
    response->resolveVersion(string_view(start, vEnd-start));
    vEnd++;

    const char* sEnd = std::find(vEnd, end, ' ');
    if(sEnd == end){
        return false;
    }
    response->setStatusCode(std::stoi(string(vEnd, sEnd-vEnd)));

    return response->getVersion() != Version::vUNKNOW;
    // return response->valid();
}

bool HttpParser::parseHeader(const char* start, const char* end){
    //string_view str = http::trim(start, end);

    const char* valueStart = std::find(start, end, ':');
    const char* itemStart = start;
    const char* itemEnd = valueStart-1;
    string_view key = http::trim(start, valueStart);
    string_view value = http::trim(valueStart+1, end);
    if(key.length() > 0){
        message_->setHeaderValue(string(key), string(value));
        return true;
    }else{
        return false;
    }
    
}
HttpParser::ParserState HttpParser::freshBodyTypeStatus(){
    bodyType_ = message_->getBodyType();
    if(bodyType_ == BodyType::bNoBody){
        return sGotAll;
    }else if(bodyType_ == BodyType::bContentLength){
        remainingBlockLen_ = static_cast<size_t>(atol(message_->getHeaderValue("Content-Length").c_str()));
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
        abort();
    }
}


bool HttpParser::parseRequest(ConnBuffer* buf, TimeStamp time){
    if(!message_){
        message_.reset(new HttpRequest(time));
        assert(messageType_ != tResponse);
        messageType_ = tRequest;
    }
    return parseLoop(buf);
}
bool HttpParser::parseResponse(ConnBuffer* buf){
    if(!message_){
        message_.reset(new HttpResponse());
        assert(messageType_ != tRequest);
        messageType_ = tResponse;
    }
    return parseLoop(buf);
}
bool HttpParser::parseLoop(ConnBuffer* buf){
    bool hasMore = buf->readableBytes() > 0;
    bool valid = true;
    while(hasMore){
        if(state_ == sExpectLine){
            const char* end = buf->findCRLF(buf->readerBegin());
            if(end==NULL){
                hasMore = false;
            }else if(parseLine(buf->readerBegin(), end)){
                state_ = sExpectHeader;
                buf->retrieveTo(end+2);
            }else{
                hasMore = false;
                valid = false;
            }
        }else if(state_ == sExpectHeader){
            const char* end = buf->findCRLF(buf->readerBegin());
            if(end==NULL){
                hasMore = false;
            }else if(end == buf->readerBegin()){
                state_ = freshBodyTypeStatus();
                buf->retrieveTo(end+2);
            }else if(parseHeader(buf->readerBegin(), end)){
                buf->retrieveTo(end+2);
            }else{
                hasMore = false;
                valid = false;
            }
        }else if(state_ == sExpectConetentBody){
            if(bodyType_ == BodyType::bContentLength){
                size_t getLen = min(buf->readableBytes(), remainingBodyLen_);
                message_->appendBody(string_view(buf->readerBegin(), getLen));
                buf->retrieve(getLen);
                remainingBodyLen_ -= getLen;
                state_ = (remainingBodyLen_==0)?sGotAll:sExpectConetentBody;
                hasMore = (remainingBodyLen_==0)?false:true;
            }else if(bodyType_ == BodyType::bUntilClosed){
                size_t getLen = buf->readableBytes();
                message_->appendBody(string_view(buf->readerBegin(), getLen));
                buf->retrieve(getLen);
            }
        }else if(state_ == sExpectBlockBodySize){
            const char* end = buf->findCRLF(buf->readerBegin());
            if(end==NULL){
                hasMore = false;
            }else{
                string blockLenString = string(buf->readerBegin(), end - buf->readerBegin());
                remainingBlockLen_ = static_cast<size_t>(strtol(blockLenString.c_str(), nullptr, 16));//16进制表达
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
                message_->appendBody(string_view(buf->readerBegin(), getLen));
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


