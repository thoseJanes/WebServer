#include "webSocketFrame.h"

using namespace mywebserver;

const WebSocketFrame::Buffer& WebSocketFrame::formatFrameToBuffer(const void* data, size_t len){
    setPayload(data, len);
    formatFrameToBuffer();
    return payload_;
}
const WebSocketFrame::Buffer& WebSocketFrame::formatFrameToBuffer(){
    payload_.retrieve(payload_.readableBytes() - payloadLength_);//先去除之前prepend的部分
    //反向前插头部
    if(mask_){
        if(!isMaskingKeySet_){
            setMaskingKey();
        }
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