#ifndef MYWEBSERVER_WEBSOCKET_WEBSOCKETPARSER_H
#define MYWEBSERVER_WEBSOCKET_WEBSOCKETPARSER_H

#include <unistd.h>
#include "webSocket.h"
#include "webSocketFrame.h"

// using namespace mynetlib;
namespace mywebserver{

class WebSocketParser{
public:
    enum ParserState{
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

    ParserState getState(){
        return state_;
    }

    bool parseFrame(mynetlib::ConnBuffer& buffer);

    void reset(){
        frame_.clear();
        state_ = sExpectFirstByte;
        remainingLen_ = 0;
    }

    const WebSocketFrame* getFrame(){
        return &frame_;
    }
private:
    void parsePayloadAndFreshState(uint8_t secondByte);

    WebSocketFrame frame_;//这个frame_可以重复使用。只要使用时保证还没有开始接收下一帧。
    ParserState state_;
    size_t remainingLen_;
    bool isServer_;
    //如果fin为0,是不是可以把接下来的东西都放进同一帧？貌似是的。
};
}


#endif// MYWEBSERVER_WEBSOCKET_WEBSOCKETPARSER_H