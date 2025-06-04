#ifndef WEBSERVER_HTTP_HTTPSERVER_H
#define WEBSERVER_HTTP_HTTPSERVER_H
#include "../net/tcpServer.h"
#include "httpParser.h"
#include "httpResponse.h"
namespace webserver{

namespace http{
void defaultHttpCallback(const shared_ptr<TcpConnection>& conn, const HttpRequest* request);
}

class HttpServer{
public:
    typedef function<void(const shared_ptr<TcpConnection>& conn, const HttpRequest* request)> HttpCallback;
    typedef TcpServer::ThreadInitCallback ThreadInitCallback;
    HttpServer(EventLoop* baseLoop, string_view name, InetAddress serverAddress, bool reusePort = false)
    :   server_(baseLoop, name, serverAddress, reusePort),
        httpCallback_(http::defaultHttpCallback)
    {
        server_.setConnectCallback(bind(&HttpServer::onConnect, this, std::placeholders::_1));
        server_.setMessageCallback(bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }

    void start(int threadNum, ThreadInitCallback threadInitCallback = NULL){
        server_.start(threadNum, threadInitCallback);
    }

    //httpCallback会在读完消息后，在channel处理处被调用。
    void setHttpCallback(HttpCallback cb){
        httpCallback_ = cb;
    }

    ~HttpServer(){}
private:
    void onConnect(const shared_ptr<TcpConnection>& conn){
        LOG_DEBUG << "on connect!";
        if(conn->isConnected()){
            HttpParser* paser = new HttpParser();
            conn->setContext(paser);
        }else{
            HttpParser* paser = any_cast<HttpParser*>(conn->getContext());
            delete paser;
        }
    }
    
    void onMessage(const shared_ptr<TcpConnection>& conn, ConnBuffer* buffer, TimeStamp time){
        LOG_DEBUG << "on message!";
        HttpParser* paser = any_cast<HttpParser*>(conn->getContext());
        //paser->parseRequest(buffer, time);
        LOG_DEBUG << "start resolve";
        LOG_DEBUG << string(buffer->readerBegin(), buffer->readableBytes());
        bool toBeResolved = true;
        while(toBeResolved){
            if(paser->parseRequest(buffer, time)){//解析成功
                toBeResolved = false;
                if(paser->getState() == HttpParser::sGotAll){//一个请求解析完成
                    httpCallback_(conn, paser->getRequest());
                    paser->reset();
                    if(buffer->readableBytes() > 0){//是否应该继续读取？两条请求可能同时发过来？
                        toBeResolved = true;
                    }
                }else{//解析未完成

                }
            }else{//解析失败。
                toBeResolved = false;
                //关闭连接？还是先向回发送一个错误页面？
                //conn->forceClose();
            }
        }
    }
    TcpServer server_;
    HttpCallback httpCallback_;
};
}

#endif