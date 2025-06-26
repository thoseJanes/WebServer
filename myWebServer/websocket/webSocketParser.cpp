#include "webSocketParser.h"

using namespace mywebserver;


void WebSocketParser::parsePayloadAndFreshState(uint8_t secondByte){
    uint8_t payloadLen = webSocket::getPayloadLenInByte(secondByte);
    if(126 > payloadLen){
        remainingLen_ = payloadLen;
        state_ = (frame_.getMask())?sExpectMaskingKey:sExpectPayloadData;
    }else if(126 == payloadLen){
        state_ = sExpectPayloadLength16;
    }else if(127 == payloadLen){
        state_ = sExpectPayloadLength64;
    }else{
        assert(false);
    }
}

bool WebSocketParser::parseFrame(mynetlib::ConnBuffer& buffer){
    bool hasMore = buffer.readableBytes()>0;
    bool valid = true;
    while(hasMore){
        if(sExpectFirstByte == state_){
            if(buffer.readableBytes() < 1){
                break;
            }

            frame_.clear();
            frame_.setFirstByte(buffer.readByte());
            state_ = sExpectPayloadLen;
        }else if(sExpectPayloadLen == state_){
            if(buffer.readableBytes() < 1){
                break;
            }

            uint8_t secondByte = buffer.readByte();
            bool mask = (secondByte >> 7);
            if(mask != isServer_){//如果为server，则mask必为true
                hasMore = false;
                valid = false;
            }else{
                frame_.setMask(mask);
                parsePayloadAndFreshState(secondByte);
            }
        }else if(sExpectPayloadLength16 == state_){
            if(buffer.readableBytes() < 2){
                break;
            }

            remainingLen_ = buffer.readUint16();
            state_ = (frame_.getMask())?sExpectMaskingKey:sExpectPayloadData;
        }else if(sExpectPayloadLength64 == state_){
            if(buffer.readableBytes() < 8){
                break;
            }

            remainingLen_ = buffer.readUint64();
            state_ = (frame_.getMask())?sExpectMaskingKey:sExpectPayloadData;
        }else if(sExpectMaskingKey == state_){
            if(buffer.readableBytes() < 4){
                break;
            }

            uint32_t maskingKey;
            buffer.read(&maskingKey, sizeof(maskingKey));
            frame_.setMaskingKey(maskingKey);
            state_ = sExpectPayloadData;
        }else if(sExpectPayloadData == state_){
            if(buffer.readableBytes()<1){
                break;
            }

            size_t getLen = std::min(buffer.readableBytes(), remainingLen_);
            if(frame_.getMask()){
                webSocket::toUnmasked(buffer.readerBegin(), getLen, frame_.getMaskingKey());
            }
            
            frame_.appendPayload(buffer.readerBegin(), getLen);
            buffer.retrieve(getLen);
            remainingLen_ -= getLen;
            if(0 == remainingLen_){
                state_ = (frame_.getFin())?sGotAll:sExpectNextFrame;
            }
            hasMore = false;
        }else if(sExpectNextFrame == state_){
            if(buffer.readableBytes() < 1){
                break;
            }

            uint8_t firstByte = buffer.readByte();
            if((webSocket::getRsv1InByte(firstByte) == webSocket::getRsv1InByte(frame_.getFirstByte()))
            && (webSocket::getRsv2InByte(firstByte) == webSocket::getRsv2InByte(frame_.getFirstByte()))
            && (webSocket::getRsv3InByte(firstByte) == webSocket::getRsv3InByte(frame_.getFirstByte()))
            && webSocket::getOpcodeInByte(firstByte) == webSocket::frameTypeToOpcode(webSocket::tContinuationFrame)){
                frame_.setFin(webSocket::getFinInByte(firstByte));
                state_ = sExpectPayloadLen;
            }else{
                hasMore = false;
                valid = false;
            }
        }else if(sGotAll == state_){
            hasMore = false;
        }
    }
    return valid;
}