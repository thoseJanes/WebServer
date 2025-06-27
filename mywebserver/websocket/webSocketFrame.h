#ifndef MYWEBSREVER_WEBSOCKET_WEBSOCKETFRAME_H
#define MYWEBSREVER_WEBSOCKET_WEBSOCKETFRAME_H

#include <cstdint>
#include <string>
#include <string_view>

#include "../../mynetlib/net/connBuffer.h"
#include "../http/httpRequest.h"
#include "../http/httpResponse.h"
#include "webSocket.h"


namespace mywebserver{


//如何保证既能用于parser又能用于发送帧？
//仅在formatFrameToBuffer中进行自动设置即可。
class WebSocketFrame{
public:
    typedef mynetlib::ConnBuffer Buffer;
    WebSocketFrame(bool mask = false)
    :   payload_(125, 2+8+4), 
        isMaskingKeySet_(false), 
        payloadMasked_(false), 
        payloadLength_(0), 
        firstByte_(0x80){
        setMask(mask);
    };
    ~WebSocketFrame() = default;
    void clear(){
        payload_.retrieveAll();
        firstByte_ = 0x80;
        mask_ = 0;
        payloadLength_ = 0;
        payloadMasked_ = false;
        isMaskingKeySet_ = false;
    }
    
    //注意不要提供其它方法获取payload_
    const Buffer& formatFrameToBuffer(const void* data, size_t len);
    const Buffer& formatFrameToBuffer();

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
    void appendPayload(const void* data, size_t len){
        if(payloadMasked_){
            LOG_FATAL << "WebsocketFrame::appendPayload - append payload when payload has been masked";
        }
        payloadLength_ += len;
        payload_.append(data, len);
    }
    string_view getPayloadView() const {
        if(payloadMasked_){
            LOG_ERROR << "WebsocketFrame::getPayloadView - get masked payload";
        }
        return string_view(payload_.writerBegin() - payloadLength_, payloadLength_);
    }
    size_t getPayloadLength() const {
        return payloadLength_;
    }
    

    void setMask(bool mask){
        mask_ = (mask)?1:0;
    }
    void setMaskingKey(uint32_t maskingKey){
        if(__builtin_expect(isMaskingKeySet_, false)){
            LOG_WARN << "setMaskingKey - masking key has been set. overwrite the masking key before. " << " isMaskingKeySet:" << isMaskingKeySet_;
        }
        maskingKey_ = maskingKey;
        isMaskingKeySet_ = true;
    }
    //自动设置maskingKey
    void setMaskingKey(){
        if(__builtin_expect(isMaskingKeySet_, false)){
            LOG_WARN << "setMaskingKey - masking key has been set. overwrite the masking key before. " << " isMaskingKeySet:" << isMaskingKeySet_;
        }
        webSocket::generateRandBytes(&maskingKey_, sizeof(maskingKey_));
        isMaskingKeySet_ = true;
    }

    bool getMask() const {
        return mask_>0;
    }
    uint32_t getMaskingKey() const {
        if(!__builtin_expect(isMaskingKeySet_, false)){
            LOG_ERROR << "getMaskingKey - masking key has not been set." << " isMaskingKeySet:" << isMaskingKeySet_;
            return 0;
        }
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
    uint8_t getFirstByte() const {
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
            // LOG_DEBUG << "mask payload " << string_view(payload_.writerBegin() - payloadLength_, payloadLength_)
            //     << "with maskingKey:" << maskingKey_;
            char* payloadStart = payload_.writerBegin() - payloadLength_;
            webSocket::toMasked(payloadStart, payloadLength_, maskingKey_);
            payloadMasked_ = true;
        }else{
            LOG_ERROR << "maskPayload - payload has been masked";
        }
    }
    uint8_t firstByte_;

    Buffer payload_;
    size_t payloadLength_;
    uint32_t maskingKey_;
    uint8_t mask_;
    bool isMaskingKeySet_;
    bool payloadMasked_;
};

}
#endif// MYWEBSREVER_WEBSOCKET_WEBSOCKETFRAME_H