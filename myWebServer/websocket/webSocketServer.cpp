#include "webSocketServer.h"
#include "webSocket.h"
#include "webSocketFrame.h"
#include "webSocketParser.h"
#include "webSocketHandshake.h"

using namespace mywebserver;


void webSocket::defaultBadFrameCallback(const shared_ptr<TcpConnection>& conn){
    LOG_ERROR << "defaultBadFrameCallback - get a bad frame from " << conn->peerAddressString();
    WebSocketFrame frame;
    frame.setFin(true);
    frame.setFrameType(webSocket::FrameType::tConnectionFrame);
    conn->send(&frame.formatFrameToBuffer());
    conn->shutdownWrite();
}

void webSocket::defaultHandshakeSuccessCallback(const shared_ptr<TcpConnection>& conn){
    WebSocketFrame frame;
    frame.setFrameType(webSocket::FrameType::tPingFrame);
    conn->send(&frame.formatFrameToBuffer());
    LOG_DEBUG << "defaultHandshakeSuccessCallback - send a ping frame";
}

void webSocket::defaultWebSocketCallback(const shared_ptr<TcpConnection>& conn, const WebSocketFrame* frame, ContextMap& context){
    LOG_DEBUG << "server get frame with type " << webSocket::frameTypeToString(frame->getFrameType());
    LOG_DEBUG << "defaultWebSocketCallback - get a frame with payload:\n" << frame->getPayloadView() << "\nwith maskingKey:" << frame->getMaskingKey();
    if(frame->getFrameType() == webSocket::FrameType::tPingFrame){
        WebSocketFrame frameOut;
        frameOut.setFrameType(webSocket::FrameType::tPongFrame);
        conn->send(&frameOut.formatFrameToBuffer());
    }else if(frame->getFrameType() == webSocket::FrameType::tConnectionFrame){
        LOG_DEBUG << "server get a close frame";
        conn->shutdownWrite();
    }
}


void WebSocketServer::onHandshake(const shared_ptr<TcpConnection>& conn, ConnBuffer* buffer, TimeStamp time, ContextMap* context){
    if(!__builtin_expect(context->hasContext("httpParser"), true)){
        sendHttpResponse400AndShutdownWrite(conn);
        return;
    }

    HttpParser* parser = static_cast<HttpParser*>(context->getContext("httpParser"));
    if(!parser->parseRequest(buffer, time)){
        sendHttpResponse400AndShutdownWrite(conn);
        return;
    }

    if(parser->getState() != HttpParser::sGotAll){
        return;
    }

    auto request = parser->getRequest();
    LOG_DEBUG << "server "<<server_.getName()<<" get request " << request->toString();
    if(!webSocket::isWebSocketHandshakeRequestValid(*request)){
        sendHttpResponse400AndShutdownWrite(conn);
        return;
    }

    WebSocketHandshakeResponse handshakeResponse(*request);
    conn->send(handshakeResponse.toString());

    delete parser;
    context->removeContext("httpParser");
    WebSocketParser* wsparser = new WebSocketParser(true);
    context->setContext("webSocketParser", wsparser);
    handshakeCallback_(conn);
}
void WebSocketServer::onMessage(const shared_ptr<TcpConnection>& conn, ConnBuffer* buffer, TimeStamp time){
    LOG_DEBUG << "server " << server_.getName() << " on message";
    ContextMap* context = std::any_cast<ContextMap*>(conn->getContext());
    if(!__builtin_expect(context->hasContext("webSocketParser"), true)){
        onHandshake(conn, buffer, time, context);
    }
    //为了防止在上面的http请求之后还跟着frame，不能用else。
    if(__builtin_expect(context->hasContext("webSocketParser"), true)){
        WebSocketParser* parser = static_cast<WebSocketParser*>(context->getContext("webSocketParser"));
        while(true){
            if(!parser->parseFrame(*buffer)){
                badFrameCallback_(conn);
                return;
            }

            if(parser->getState() != WebSocketParser::ParserState::sGotAll){
                return;
            }

            // WebSocketFrame frameOut;
            const WebSocketFrame* frame = parser->getFrame();
            webSocketCallback_(conn, frame, *context);
            // conn->send(&frameOut.formatFrameToBuffer());
            parser->reset();
        }
    }
}
void WebSocketServer::onConnection(const shared_ptr<TcpConnection>& conn){
    if(conn->isConnected()){
        HttpParser* parser = new HttpParser();
        ContextMap* context = new ContextMap();
        context->setContext("httpParser", parser);
        conn->setContext(context);
        LOG_INFO << "connect from address:" << conn->peerAddressString() << ", present number of connections:" << this->server_.getConnectionNum();
    }else{
        ContextMap* context = std::any_cast<ContextMap*>(conn->getContext());
        if(context->hasContext("httpParser")){
            HttpParser* parser = static_cast<HttpParser*>(context->getContext("httpParser"));
            delete parser;
        }
        if(context->hasContext("webSocketParser")){
            WebSocketParser* parser = static_cast<WebSocketParser*>(context->getContext("webSocketParser"));
            delete parser;
        }
    }
}