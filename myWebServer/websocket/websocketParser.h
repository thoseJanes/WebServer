#ifndef MYWEBSERVER_WEBSOCKET_WEBSOCKETPARSER_H
#define MYWEBSERVER_WEBSOCKET_WEBSOCKETPARSER_H

#include <unistd.h>
#include "websocketFrame.h"

// using namespace mynetlib;
namespace mywebserver{

class WebSocketParser{
public:
    enum PaserState{
        sExpectFirstByte,//one byte
        sExpectPayloadLen,//one byte
        sExpectPayloadLength16,//two bytes
        sExpectPayloadLength64,//eight bytes
        sExpectMaskingKey,//four bytes
        sExpectPayloadData,//any
        sExpectNextFrame,
        sGotAll
    };
    WebSocketParser(bool isServer):state_(sExpectFirstByte), isServer_(isServer), remainingLen_(0){}
    ~WebSocketParser() = default;

    void parsePayloadAndFreshState(uint8_t secondByte){
        uint8_t payloadLen = secondByte & (~0x80);
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

    bool parseFrame(mynetlib::ConnBuffer buffer){
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
                remainingLen_ -= getLen;
                if(0 == remainingLen_){
                    state_ = (frame_.getFin())?sGotAll:sExpectNextFrame;
                }
                hasMore = false;
            }else if(sExpectNextFrame == state_){
                if(buffer.readableBytes() < 1){
                    break;
                }

                uint8_t firstByte = buffer.readableBytes();
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
            }
        }
        return valid;
    }

    bool reset(){
        frame_.clear();
        state_ = sExpectFirstByte;
        remainingLen_ = 0;
    }
private:
    WebSocketFrame frame_;//这个frame_可以重复使用。只要使用时保证还没有开始接收下一帧。
    PaserState state_;
    size_t remainingLen_;
    bool isServer_;
    //如果fin为0,是不是可以把接下来的东西都放进同一帧？貌似是的。
};
}


#endif// MYWEBSERVER_WEBSOCKET_WEBSOCKETPARSER_H