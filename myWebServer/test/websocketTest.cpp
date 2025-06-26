#include "../websocket/webSocketServer.h"
#include "../../myNetLib/net/tcpClient.h"
#include <any>


using namespace mywebserver;

//因为客户端最多建立一个连接，所以可以直接保留这些context，不需要为每个连接都建立一个？
void clientConnectCallback(const shared_ptr<TcpConnection>& conn){
    if(conn->isConnected()){
        LOG_INFO << "connected";
        ContextMap* map = new ContextMap();
        conn->setContext(map);

        ClientWebSocketHandShakeContext* clientContext = new ClientWebSocketHandShakeContext();
        clientContext->generateRequestKey();
        map->setContext("handShakeContext", clientContext);
        map->setContext("httpParser", new HttpParser());
        conn->send(clientContext->getRequestString());
        LOG_INFO << "send " << clientContext->getRequestString();
    }else{
        ContextMap* context = std::any_cast<ContextMap*>(conn->getContext());
        if(context->hasContext("handShakeContext")){
            ClientWebSocketHandShakeContext* handShakeContext = static_cast<ClientWebSocketHandShakeContext*>(context->getContext("handShakeContext"));
        }
        if(context->hasContext("httpParser")){
            HttpParser* httpParser = static_cast<HttpParser*>(context->getContext("httpParser"));
        }
        if(context->hasContext("webSocketParser")){
            WebSocketParser* webSocketParser = static_cast<WebSocketParser*>(context->getContext("webSocketParser"));
        }
    }
}

void clientMessageCallback(const shared_ptr<TcpConnection>& conn, ConnBuffer* buffer, TimeStamp time){
    ContextMap* context = std::any_cast<ContextMap*>(conn->getContext());
    if(__builtin_expect(context->hasContext("httpParser"), false)){
        HttpParser* parser = static_cast<HttpParser*>(context->getContext("httpParser"));
        //这并非响应parser，而是请求parser。
        if(!parser->parseResponse(buffer)){
            conn->shutdownWrite();//对于客户端，是不是应该直接forceClose？
            return;
        }
        if(!parser->getState() == mywebserver::HttpParser::sGotAll){
            return;
        }
        const HttpResponse* response = parser->getResponse();
        ClientWebSocketHandShakeContext* handShakeContext = static_cast<ClientWebSocketHandShakeContext*>(context->getContext("handShakeContext"));
        bool valid = handShakeContext->isResponseValid(*response);
        if(!valid){
            conn->shutdownWrite();
            return;
        }

        delete handShakeContext;
        delete parser;
        context->removeContext("httpParser");
        context->removeContext("handShakeContext");
        context->setContext("webSocketParser", new WebSocketParser(false));
        
    }
    //为了防止上面的握手之后还跟着frame，不能用else。
    if(__builtin_expect(context->hasContext("webSocketParser"), true)){
        WebSocketParser* parser = static_cast<WebSocketParser*>(context->getContext("webSocketParser"));
        if(!parser->parseFrame(*buffer)){
            WebSocketFrame frame;
            frame.setFrameType(mywebserver::webSocket::tConnectionFrame);
            conn->send(&frame.formatFrameToBuffer());
            conn->shutdownWrite();
            return;
        }

        if(parser->getState() != WebSocketParser::ParserState::sGotAll){
            return;
        }

        auto frame = parser->getFrame();
        // WebSocketFrame frameOut;
        LOG_INFO << "defaultWebSocketCallback - get a frame with payload:\n" << frame->getPayloadView();
        if(frame->getFrameType() == webSocket::FrameType::tPingFrame){
            WebSocketFrame frameOut;
            frameOut.setFrameType(webSocket::FrameType::tPingFrame);
            conn->send(&frameOut.formatFrameToBuffer());
        }else if(frame->getFrameType() == webSocket::FrameType::tConnectionFrame){
            conn->shutdownWrite();
        }
        // conn->send(&frameOut.formatFrameToBuffer());
        parser->reset();
    }
}

int main(){
    auto ret = fork();
    InetAddress serverAddress("127.0.0.1", 8000, AF_INET);
    if(ret != 0){
        EventLoop loop;
        WebSocketServer server(&loop, "test", serverAddress);
        server.start(4);
        loop.loop();
    }else{
        EventLoop loop;
        TcpClient client(&loop, "testClient", serverAddress);
        client.setConnectCallback(clientConnectCallback);
        client.setMessageCallback(clientMessageCallback);
        LOG_INFO << "client start";
        client.connect();
        loop.loop();
    }
}