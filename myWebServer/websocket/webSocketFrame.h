#ifndef MYWEBSREVER_WEBSOCKET_WEBSOCKETFRAME_H
#define MYWEBSREVER_WEBSOCKET_WEBSOCKETFRAME_H

#include <cstdint>
#include <string>
#include <string_view>
#include <openssl/rand.h>

#include "../../myNetLib/net/connBuffer.h"
#include "../http/httpRequest.h"
#include "../http/httpResponse.h"
#include "webSocket.h"


namespace mywebserver{



class WebSocketFrame{
public:
    typedef mynetlib::ConnBuffer Buffer;
    WebSocketFrame():mask_(0), payload_(125, 2+8+4), payloadMasked_(false), payloadLength_(0), firstByte_(0x80){
    };
    ~WebSocketFrame() = default;
    
    //注意不要提供其它方法获取payload_
    const Buffer& formatFrameToBuffer(const void* data, size_t len){
        setPayload(data, len);
        formatFrameToBuffer();
        return payload_;
    }

    const Buffer& formatFrameToBuffer(){
        payload_.retrieve(payload_.readableBytes() - payloadLength_);//先去除之前prepend的部分
        //反向前插头部
        if(mask_){
            payload_.prepend(&maskingKey_, sizeof(maskingKey_));
            if(!payloadMasked_){
                maskPayload();
            }
        }

        uint8_t payloadLen = getPayloadLen();
        uint8_t secondByte =  (mask_ << 7) | payloadLen;
        if(payloadLen == 126){
            payload_.prependUint16(static_cast<uint16_t>(payloadLength_));
        }else if(payloadLen == 127){
            payload_.prependUint64(static_cast<uint64_t>(payloadLength_));
        }
        payload_.prepend(&secondByte, sizeof(secondByte));
        //前端长度最大为2+8+4字节
        payload_.prepend(&firstByte_, sizeof(firstByte_));

        return payload_;
    }

    bool isPayloadMasked(){
        return payloadMasked_;
    }

    void setPayload(const void* data, size_t len){
        if(payloadLength_){
            payload_.retrieve(payloadLength_);
        }
        if(payloadMasked_){
            LOG_WARN << "WebsocketFrame::setPayload - set payload when payload has been masked";
            payloadMasked_ = false;
        }
        payloadLength_ = len;
        payload_.append(data, len);
    }
    void clear(){
        payload_.retrieveAll();
        mask_ = 0;
        firstByte_ = 0;
        payloadMasked_ = false;
    }

    string_view getPayloadView() const {
        if(payloadMasked_){
            LOG_ERROR << "WebsocketFrame::getPayloadView - get masked payload";
        }
        return string_view(payload_.readerBegin(), payloadLength_);
    }
    void appendPayload(const void* data, size_t len){
        if(payloadMasked_){
            LOG_FATAL << "WebsocketFrame::appendPayload - append payload when payload has been masked";
        }
        payloadLength_ += len;
        payload_.append(data, len);
    }

    void setMask(bool mask){
        mask_ = (mask)?1:0;
    }

    void setMaskingKey(uint32_t maskingKey){
        maskingKey_ = maskingKey;
    }

    bool getMask(){
        return mask_>0;
    }

    uint32_t getMaskingKey(){
        return maskingKey_;
    }

    #define SET_GET_WEBSOCKET_FIRST_BIT_GENERATOR(name)\
    uint8_t get ## name() const {\
        return webSocket::get ## name ## InByte(firstByte_);\
    }\
    void set ## name(bool val){\
        webSocket::set ## name ## InByte(val, &firstByte_);\
    }

    SET_GET_WEBSOCKET_FIRST_BIT_GENERATOR(Fin);
    SET_GET_WEBSOCKET_FIRST_BIT_GENERATOR(Rsv1);
    SET_GET_WEBSOCKET_FIRST_BIT_GENERATOR(Rsv2);
    SET_GET_WEBSOCKET_FIRST_BIT_GENERATOR(Rsv3);
    #undef SET_GET_WEBSOCKET_FIRST_BIT_GENERATOR

    void setOpcode(uint8_t val){
        webSocket::setOpcode(val, &firstByte_);
    }
    void setFrameType(webSocket::FrameType type){
        webSocket::setOpcode(webSocket::frameTypeToOpcode(type), &firstByte_); 
    }
    uint8_t getOpcode() const {
        return webSocket::getOpcodeInByte(firstByte_);
    }
    webSocket::FrameType getFrameType() const {
        return webSocket::opcodeToFrameType(getOpcode());
    }

    void setFirstByte(uint8_t val){
        firstByte_ = val;
    }
    uint8_t getFirstByte(){
        return firstByte_;
    }

private:
    int getPayloadLen(){
        if(payloadLength_ < 126){
            return payloadLength_;
        }else if(payloadLength_ >126 && payloadLength_ < 0xFFFF){
            return 126;
        }else{
            return 127;
        }
    }

    void maskPayload(){
        if(!payloadMasked_){
            char* payloadStart = payload_.writerBegin() - payloadLength_;
            webSocket::toMasked(payloadStart, payloadLength_, maskingKey_);
            payloadMasked_ = true;
        }
    }

    Buffer payload_;
    size_t payloadLength_;
    uint32_t maskingKey_;
    uint8_t mask_;
    
    uint8_t firstByte_;
    bool payloadMasked_;
};

class ClientWebSocketHandShakeContext{
public:
    ClientWebSocketHandShakeContext():request_(TimeStamp::now(), http::mGET, http::vHTTP1_1), isKeySet_(false){
        request_.setHeaderValue("Upgrade", "websocket");
        request_.setHeaderValue("Connection", "Upgrade");
        request_.setHeaderValue("Sec-WebSocket-Version", "13");
    }
    // ClientWebSocketHandShakeContext(HttpRequest request):request_(request), isKeySet_(true){
    //     assert(request_.getHeaderValue("Sec-WebSocket-Key").size() == webSocket::kKeyLength_);
    //     assert(!request_.getHeaderValue("Sec-WebSocket-Version").empty());
    //     assert(request_.getHeaderValue("Upgrade") == "websocket");
    //     assert(request_.getHeaderValue("Connection") == "Upgrade");
    // }

    // void setWebSocketKey(unsigned char key[webSocket::kOrgKeyLength_]){
    //     memcpy(webSocketKey_, key, webSocket::kOrgKeyLength_);
    //     isKeySet_ = true;
    // }
    void generateRequestKey(){
        unsigned char orgRandKey[webSocket::kOrgKeyLength_];
        RAND_bytes(orgRandKey, webSocket::kOrgKeyLength_);

        size_t len = webSocket::encodeBase64(orgRandKey, webSocket::kOrgKeyLength_, 
                                            webSocketKey_, sizeof(webSocketKey_));
        assert(len == sizeof(webSocketKey_));
        request_.setHeaderValue("Sec-WebSocket-Key", string(webSocketKey_, webSocket::kKeyLength_));
        isKeySet_ = true;
    }
    string getRequestKey(){
        if(isKeySet_){
            return string(webSocketKey_, webSocket::kKeyLength_);
            //memcpy(key, webSocketKey_, webSocket::kKeyLength_);
        }else{
            LOG_ERROR << "WebSocketHandShakeRequest::getKey - key has not been set";
            return "";
        }
    }
    void setRequestProtocol(const string& str){
        request_.setHeaderValue("Sec-WebSocket-Protocol", str);
    }
    void setRequestExtensions(const string& str){
        request_.setHeaderValue("Sec-WebSocket-Extensions", str);
    }

    std::string getRequestString(){
        if(!isKeySet_){
            generateRequestKey();
        }
        return request_.toString();
    }
    const HttpRequest& getHttpRequest(){
        if(!isKeySet_){
            generateRequestKey();
        }
        return request_;
    }

    bool isResponseValid(const HttpResponse& response){
        return webSocket::isWebSocketHandShakeResponseValid(request_, response);
    }
private:
    // void formatKey(char* buf){
    //     char* pos = buf;
    //     for(int i=0;i<webSocket::kKeyLength_;i++){
    //         size_t ret = detail::formatInteger<unsigned char>(pos, webSocketKey_[i]);
    //         assert(ret == 1);
    //         pos++;
    //     }
    // }
    char webSocketKey_[webSocket::kKeyLength_];//即16除3向上取整再乘4.
    bool isKeySet_;
private:
    HttpRequest request_;
};

class WebSocketHandShakeResponse{
public:
    explicit WebSocketHandShakeResponse(const HttpRequest& request):response_(&request){
        assert(webSocket::isWebSocketHandShakeRequestValid(request));
        response_.setStatusCode(101);
        response_.setHeaderValue("Upgrade", "websocket");
        response_.setHeaderValue("Connection", "Upgrade");
        response_.setHeaderValue("Sec-WebSocket-Version", request.getHeaderValue("Sec-WebSocket-Version"));

        string acceptKey = webSocket::calAcceptKey(request.getHeaderValue("Sec-WebSocket-Key"));
        response_.setHeaderValue("Sec-WebSocket-Accept", acceptKey);
        //处理扩展和协议。
        handleRequestProtocol(request);
        handleRequestExtensions(request);
    }

    void handleRequestProtocol(const HttpRequest& request){
        //TOBECOMPLETED
    }
    void handleRequestExtensions(const HttpRequest& request){
        //TOBECOMPLETED
    }

    string toString(){
        return response_.toString();
    }
private:
    HttpResponse response_;
};

}
#endif// MYWEBSREVER_WEBSOCKET_WEBSOCKETFRAME_H