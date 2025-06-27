#ifndef MYWEBSERVER_WEBSOCKET_WEBSOCKETSERVER_H
#define MYWEBSERVER_WEBSOCKET_WEBSOCKETSERVER_H

#include "../../myNetLib/net/tcpServer.h"
#include "../http/httpParser.h"
#include "../http/httpRequest.h"
#include "../http/httpResponse.h"
#include "../http/contextMap.h"
#include "webSocket.h"
#include "webSocketParser.h"
#include "webSocketFrame.h"
#include <any>
#include <memory>

using namespace mynetlib;

namespace mywebserver{

namespace webSocket{

void defaultBadFrameCallback(const shared_ptr<TcpConnection>& conn);

void defaultHandshakeSuccessCallback(const shared_ptr<TcpConnection>& conn);

void defaultWebSocketCallback(const shared_ptr<TcpConnection>& conn, const WebSocketFrame* frame, ContextMap& context);


    
}

//目前只实现了和httpServer类似的消息接收方式。如果要定时发送消息呢？需要获得connection？
//可能有这样的情况：某些连接由于其帧无法解析被服务器关闭了，但是用户感知不到这一结果。如果用service处理连接可能会出现这种情况？
//除非，握手成功后就全由service包办。帧解析也包含在内？或者提供连接关闭回调？
//此外，如果要在握手成功后立即推送信息，仍需要回调。
class WebSocketServer{
public:
    typedef function<void(const shared_ptr<TcpConnection>&, const WebSocketFrame*, ContextMap&)> WebSocketCallback;
    typedef function<void(const shared_ptr<TcpConnection>&)> BadFrameCallback;
    typedef function<void(const shared_ptr<TcpConnection>&)> HandshakeSuccessCallback;
    WebSocketServer(EventLoop* baseLoop, const string& name, InetAddress address)
        :server_(baseLoop, name, address, false),
        badFrameCallback_(webSocket::defaultBadFrameCallback),
        handshakeCallback_(webSocket::defaultHandshakeSuccessCallback),
        webSocketCallback_(webSocket::defaultWebSocketCallback)
    {
        server_.setConnectCallback(std::bind(&WebSocketServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(std::bind(&WebSocketServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }
    ~WebSocketServer() = default;

    void start(int threadNum, EventThread::ThreadInitCallback threadInitCallback = NULL){
        server_.start(threadNum, threadInitCallback);
    }

    void setWebSocketCallback(WebSocketCallback cb){
        webSocketCallback_ = cb;
    }

    void setBadFrameCallback(BadFrameCallback cb){
        badFrameCallback_ = cb;
    }

    void setHandshakeSuccessCallback(HandshakeSuccessCallback cb){
        handshakeCallback_ = cb;
    }
private:
    void onConnection(const shared_ptr<TcpConnection>& conn);
    void onMessage(const shared_ptr<TcpConnection>& conn, ConnBuffer* buffer, TimeStamp time);
    void onHandshake(const shared_ptr<TcpConnection>& conn, ConnBuffer* buffer, TimeStamp time, ContextMap* context);
    void sendHttpResponse400AndShutdownWrite(const shared_ptr<TcpConnection>& conn){
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdownWrite();
    }

    void sendCloseFrameAndShutdownWrite(const shared_ptr<TcpConnection>& conn){
        WebSocketFrame frame;
        frame.setFin(true);
        frame.setFrameType(webSocket::FrameType::tConnectionFrame);
        conn->send(&frame.formatFrameToBuffer());
        conn->shutdownWrite();
    }

    TcpServer server_;
    WebSocketCallback webSocketCallback_;
    BadFrameCallback badFrameCallback_;
    HandshakeSuccessCallback handshakeCallback_;

    //WebSocketService service_;//继承该类并重写方法？否，可以绑定到回调上。
};
}

#endif// MYWEBSERVER_WEBSOCKET_WEBSOCKETSERVER_H


