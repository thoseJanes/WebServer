#include "httpParser.h"

using namespace webserver;

bool HttpParser::parseRequestLine(const char* start, const char* end){
    const char* mEnd = std::find(start, end, ' ');
    if(mEnd == end){
        return false;
    }
    request_->resolveMethod(string_view(start, mEnd-start));
    mEnd++;

    const char* uEnd = std::find(mEnd, end, ' ');
    if(uEnd == end){
        return false;
    }
    const char* meStart = std::find(mEnd, uEnd, '?');
    request_->setPath(string_view(mEnd, meStart-mEnd));
    if(meStart != uEnd){
        meStart++;
        request_->setMessage(string_view(meStart, uEnd-meStart));
    }
    uEnd++;

    request_->resolveVersion(string_view(uEnd, end-uEnd));

    return request_->valid();
}
bool HttpParser::parseRequestHeader(const char* start, const char* end){
    //string_view str = http::trim(start, end);

    const char* valueStart = std::find(start, end, ':');
    const char* itemStart = start;
    const char* itemEnd = valueStart-1;
    string_view key = http::trim(start, valueStart);
    string_view value = http::trim(valueStart+1, end);
    if(key.length() > 0){
        request_->setHeaderValue(string(key), string(value));
        return true;
    }else{
        return false;
    }
    
}
HttpParser::ParserState HttpParser::freshBodyTypeStatus(){
    bodyType_ = request_->getBodyType();
    if(bodyType_ == BodyType::bNoBody){
        return sGotAll;
    }else if(bodyType_ == BodyType::bContentLength){
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
        abort();
    }
}

bool HttpParser::parseRequest(ConnBuffer* buf, TimeStamp time){
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
            }else if(parseRequestLine(buf->readerBegin(), end)){
                
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
