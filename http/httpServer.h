#ifndef WEBSERVER_HTTP_HTTPSERVER_H
#define WEBSERVER_HTTP_HTTPSERVER_H
#include "../net/tcpServer.h"
#include "httpParser.h"
#include "httpResponse.h"
namespace webserver{
class HttpServer{
public:
    HttpServer(EventLoop* baseLoop, string_view name, InetAddress serverAddress, bool reusePort)
    :   server_(baseLoop, name, serverAddress, reusePort)
    {
        server_.setConnectCallback(bind(&HttpServer::onConnect, this, std::placeholders::_1));
        server_.setMessageCallback(bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    }

    void onConnect(const shared_ptr<TcpConnection>& conn){
        if(conn->isConnected()){
            HttpParser* paser = new HttpParser();
            conn->setContext(paser);
        }else{
            HttpParser* paser = any_cast<HttpParser*>(conn->getContext());
            delete paser;
        }
    }
    
    void onMessage(const shared_ptr<TcpConnection>& conn, ConnBuffer* buffer, TimeStamp time){
        HttpParser* paser = any_cast<HttpParser*>(conn->getContext());
        paser->parseRequest(buffer, time);

        HttpResponse response = HttpResponse();

        conn->send(response.toString());
    }

    ~HttpServer();
    
private:
    TcpServer server_;
};
}

#endif