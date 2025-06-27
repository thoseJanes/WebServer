#include "../websocket/webSocketServer.h"
#include "../../myNetLib/net/tcpClient.h"
#include "../websocket/webSocketHandshake.h"
#include <any>


using namespace mywebserver;
void sendPingFrameInMs(const shared_ptr<TcpConnection>& conn, int delayMs){
    WebSocketFrame frameOut(true);
    frameOut.setFrameType(webSocket::tPingFrame);

    string timeString = TimeStamp::now().toFormattedString();
    frameOut.setPayload(timeString.c_str(), timeString.size());
    LOG_DEBUG << "client will send ping frame:"
            << "\norigin:" << frameOut.getPayloadView()
            << "\nmasked:" << frameOut.formatFrameToBuffer().toString() << "\npayload length:" << frameOut.getPayloadView().size()
            << "\nmasking key:" << frameOut.getMaskingKey();
    conn->sendDelay(frameOut.formatFrameToBuffer().toString(), delayMs);
}
//因为客户端最多建立一个连接，所以可以直接保留这些context，不需要为每个连接都建立一个？
void clientConnectCallback(const shared_ptr<TcpConnection>& conn){
    if(conn->isConnected()){
        LOG_INFO << "connected";
        ContextMap* map = new ContextMap();
        conn->setContext(map);

        ClientWebSocketHandshakeContext* clientContext = new ClientWebSocketHandshakeContext();
        clientContext->generateRequestKey();
        map->setContext("handshakeContext", clientContext);
        map->setContext("httpParser", new HttpParser());
        conn->send(clientContext->getRequestString());
        LOG_INFO << "client send handshake " << clientContext->getRequestString();

    }else{
        ContextMap* context = std::any_cast<ContextMap*>(conn->getContext());
        if(context->hasContext("handshakeContext")){
            ClientWebSocketHandshakeContext* handshakeContext = static_cast<ClientWebSocketHandshakeContext*>(context->getContext("handshakeContext"));
        }
        if(context->hasContext("httpParser")){
            HttpParser* httpParser = static_cast<HttpParser*>(context->getContext("httpParser"));
            delete httpParser;
        }
        if(context->hasContext("webSocketParser")){
            WebSocketParser* webSocketParser = static_cast<WebSocketParser*>(context->getContext("webSocketParser"));
            delete webSocketParser;
        }
    }
}

void handleHandshakeResponse(const shared_ptr<TcpConnection>& conn, ConnBuffer* buffer){
    ContextMap* context = std::any_cast<ContextMap*>(conn->getContext());
    HttpParser* parser = static_cast<HttpParser*>(context->getContext("httpParser"));
    //这并非响应parser，而是请求parser。
    if(!parser->parseResponse(buffer)){
        LOG_INFO << "client get a bad response";
        conn->shutdownWrite();//对于客户端，是不是应该直接forceClose？
        return;
    }
    if(!parser->getState() == mywebserver::HttpParser::sGotAll){
        return;
    }
    
    const HttpResponse* response = parser->getResponse();
    LOG_INFO << "client get handshake: " << response->getStatusCode();
    ClientWebSocketHandshakeContext* handshakeContext = static_cast<ClientWebSocketHandshakeContext*>(context->getContext("handshakeContext"));
    bool valid = handshakeContext->isResponseValid(*response);
    if(!valid){
        conn->shutdownWrite();
        return;
    }

    delete handshakeContext;
    delete parser;
    context->removeContext("httpParser");
    context->removeContext("handshakeContext");
    context->setContext("webSocketParser", new WebSocketParser(false));
    LOG_INFO << "succeed in shaking hands from client";
}

void clientMessageCallback(const shared_ptr<TcpConnection>& conn, ConnBuffer* buffer, TimeStamp time){
    ContextMap* context = std::any_cast<ContextMap*>(conn->getContext());
    if(__builtin_expect(context->hasContext("httpParser"), false)){
        handleHandshakeResponse(conn, buffer);
        if(__builtin_expect(context->hasContext("webSocketParser"), false)){
            sendPingFrameInMs(conn, 3000);
        }
    }
    
    //为了防止上面的握手之后还跟着frame，不能用else。
    if(__builtin_expect(context->hasContext("webSocketParser"), true)){
        WebSocketParser* parser = static_cast<WebSocketParser*>(context->getContext("webSocketParser"));
        if(!parser->parseFrame(*buffer)){
            LOG_INFO << "client get a bad frame";
            WebSocketFrame frame(true);
            frame.setFrameType(mywebserver::webSocket::tConnectionFrame);
            conn->send(&frame.formatFrameToBuffer());
            LOG_INFO << "client send a close frame";
            conn->shutdownWrite();
            return;
        }

        if(parser->getState() != WebSocketParser::ParserState::sGotAll){
            return;
        }

        auto frame = parser->getFrame();
        LOG_INFO << "client get frame with type " << webSocket::frameTypeToString(frame->getFrameType());
        // WebSocketFrame frameOut;
        LOG_INFO << "client get a frame with payload:\n" << frame->getPayloadView()
                << "\npayloadLength:" << frame->getPayloadView().length();
        if(frame->getFrameType() == webSocket::FrameType::tPingFrame){
            WebSocketFrame frameOut(true);
            frameOut.setFrameType(webSocket::FrameType::tPongFrame);
            conn->send(&frameOut.formatFrameToBuffer());
            LOG_INFO << "client send a pong frame";
        }else if(frame->getFrameType() == webSocket::FrameType::tConnectionFrame){
            LOG_INFO << "client get a close frame";
            conn->shutdownWrite();
        }else if(frame->getFrameType() == webSocket::FrameType::tPongFrame){
            LOG_INFO << "client get a pong frame";
            sendPingFrameInMs(conn, 3000);
        }
        // conn->send(&frameOut.formatFrameToBuffer());
        parser->reset();
    }
}



int main(){
    Global::setGlobalLogLevel(Logger::Debug);
    auto ret = fork();
    InetAddress serverAddress("127.0.0.1", 8000, AF_INET);
    if(ret != 0){
        EventLoop loop;
        WebSocketServer server(&loop, "test", serverAddress);
        server.start(4);
        //loop.runAfter(60000, [](){abort();});
        loop.loop();
    }else{
        int clientNum = 1;
        int forkTimes = 0;
        while(clientNum >> (forkTimes+1) > 0){
            forkTimes ++;
        }
        for(int i=0;i<forkTimes;i++){
            int ret = fork();
            if(ret != 0 && (clientNum & (1<<(forkTimes-i-1)))){
                fork();
            }
        }

        EventLoop loop;
        TcpClient client(&loop, "testClient", serverAddress);
        client.setConnectCallback(clientConnectCallback);
        client.setMessageCallback(clientMessageCallback);
        LOG_INFO << "client start";
        client.connect();
        loop.runAfter(60000, [](){abort();});
        loop.loop();
    }
}