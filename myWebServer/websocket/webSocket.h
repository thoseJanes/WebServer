#ifndef MYWEBSERVER_WEBSOCKET_WEBSOCKET_H
#define MYWEBSERVER_WEBSOCKET_WEBSOCKET_H

#include <cstdint>
#include <unistd.h>
#include <string>
#include <assert.h>
#include <string.h>
#include <openssl/sha.h>



using namespace std;
namespace mywebserver{

class HttpRequest;
class HttpResponse;

namespace webSocket{
    constexpr int kOrgKeyLength_ = 16;
    constexpr int kKeyLength_ = ((webSocket::kOrgKeyLength_-1)/3+1)*4;
    constexpr size_t kAcceptKeyLength = ((SHA_DIGEST_LENGTH-1)/3+1)*4;

    inline constexpr uint8_t kFinPos_ = 0x80;
    inline constexpr uint8_t kRsv1Pos_ = 0x40;
    inline constexpr uint8_t kRsv2Pos_ = 0x20;
    inline constexpr uint8_t kRsv3Pos_ = 0x10;
    inline constexpr uint8_t kOpcodePos_ = 0x0F;
    inline constexpr uint8_t kPayloadPos_ = 0x7F;
    inline constexpr uint8_t kMaskPos_ = 0x80;

    enum FrameType{
        tContinuationFrame = 0,
        tTextFrame = 1,
        tBinaryFrame = 2,
        //控制类型
        tConnectionFrame = 8,
        tPingFrame = 9,
        tPongFrame = 10,
    };

    #define SET_GET_WEBSOCKET_BIT_GENERATOR(name)\
    inline uint8_t get ## name ## InByte(uint8_t firstByte){\
        return (firstByte & webSocket::k ## name ## Pos_)?1:0;\
    }\
    inline void set ## name ## InByte(bool val, uint8_t* firstByte){\
        if (val){\
            *firstByte = *firstByte | webSocket::k ## name ## Pos_; \
        }else{\
            *firstByte = *firstByte & ( ~ webSocket::k ## name ## Pos_ );\
        }\
    }

    SET_GET_WEBSOCKET_BIT_GENERATOR(Fin);
    SET_GET_WEBSOCKET_BIT_GENERATOR(Rsv1);
    SET_GET_WEBSOCKET_BIT_GENERATOR(Rsv2);
    SET_GET_WEBSOCKET_BIT_GENERATOR(Rsv3);
    #undef SET_GET_WEBSOCKET_BIT_GENERATOR

    inline uint8_t getPayloadLenInByte(uint8_t secondByte){
        return secondByte & kPayloadPos_;
    }

    inline uint8_t getMaskInByte(uint8_t secondByte){
        return (secondByte & kMaskPos_)?1:0;
    }

    inline uint8_t getOpcodeInByte(uint8_t fristByte){
        return fristByte & kOpcodePos_;
    }

    inline FrameType opcodeToFrameType(uint8_t opcode){
        return static_cast<FrameType>(opcode);
    }

    inline uint8_t frameTypeToOpcode(FrameType type){
        return static_cast<uint8_t>(type);
    }

    inline void setOpcode(uint8_t val, uint8_t* firstByte){
        assert(val <= 0x0F);
        *firstByte = (*firstByte & (~webSocket::kOpcodePos_)) | val;
    }

    inline void toMasked(char* buf, size_t len, uint32_t maskingKey){
        for(int i=0;i<len;i++){
            int j = i%4;
            uint8_t maskingByte = maskingKey >> (j*8);
            *buf = (*buf)^static_cast<char>(maskingByte);//异或做掩码。
            buf++;
        }
    }

    inline void toUnmasked(char* buf, size_t len, uint32_t maskingKey){
        toMasked(buf, len, maskingKey);
    }

    
    bool isWebSocketHandShakeRequestValid(const HttpRequest& request);
    bool isWebSocketHandShakeResponseValid(const HttpRequest& request, const HttpResponse& response);


    constexpr char magicNumber[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    string calAcceptKey(const string& webSocketKey);
    size_t encodeBase64(void* input, size_t inputSize, void* output, size_t outputSize);
    size_t decodeBase64(void* input, size_t inputSize, void* output, size_t outputSize);

}
}
#endif// MYWEBSERVER_WEBSOCKET_WEBSOCKET_H