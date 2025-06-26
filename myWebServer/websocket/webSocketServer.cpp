#include "webSocketServer.h"

using namespace mywebserver;

void WebSocketServer::onHandShake(const shared_ptr<TcpConnection>& conn, ConnBuffer* buffer, TimeStamp time, ContextMap* context){
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
    if(!webSocket::isWebSocketHandShakeRequestValid(*request)){
        sendHttpResponse400AndShutdownWrite(conn);
        return;
    }

    WebSocketHandShakeResponse handShakeResponse(*request);
    conn->send(handShakeResponse.toString());

    delete parser;
    context->removeContext("httpParser");
    WebSocketParser* wsparser = new WebSocketParser(true);
    context->setContext("webSocketParser", wsparser);
    handShakeCallback_(conn);
}


void WebSocketServer::onMessage(const shared_ptr<TcpConnection>& conn, ConnBuffer* buffer, TimeStamp time){
    ContextMap* context = std::any_cast<ContextMap*>(conn->getContext());
    if(!__builtin_expect(context->hasContext("webSocketParser"), true)){
        onHandShake(conn, buffer, time, context);
    }
    //为了防止在上面的http请求之后还跟着frame，不能用else。
    if(__builtin_expect(context->hasContext("webSocketParser"), true)){
        WebSocketParser* parser = std::any_cast<WebSocketParser*>(context->getContext("webSocketParser"));
        while(true){
            if(!parser->parseFrame(*buffer)){
                badFrameCallback_(conn);
                return;
            }

            if(parser->getState() != WebSocketParser::ParserState::sGotAll){
                return;
            }

            // WebSocketFrame frameOut;
            webSocketCallback_(conn, parser->getFrame(), *context);
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
    }else{
        ContextMap* context = std::any_cast<ContextMap*>(conn->getContext());
        if(context->hasContext("httpParser")){
            HttpParser* parser = static_cast<HttpParser*>(context->getContext("httpParser"));
            delete parser;
        }
        if(context->hasContext("webSocketParser")){
            HttpParser* parser = static_cast<HttpParser*>(context->getContext("webSocketParser"));
            delete parser;
        }
    }
}