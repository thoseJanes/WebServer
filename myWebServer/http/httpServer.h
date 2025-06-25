#ifndef MYWEBSERVER_HTTP_HTTPSERVER_H
#define MYWEBSERVER_HTTP_HTTPSERVER_H
#include "../../myNetLib/net/tcpServer.h"
#include "httpParser.h"
#include "httpResponse.h"


namespace mywebserver{

class ContextMap:Noncopyable{
public:
    void setContext(string key, void* value) {
        assert(!hasContext(key));
        context_.insert({key, value});
    }
    bool hasContext(string key) {
        return context_.find(key) != context_.end();
    }
    void* getContext(string key) {
        return context_.at(key);
    }
    void removeContext(string key){
        auto ret = context_.erase(key);
        assert(ret == 1);
    }
private:
    std::map<string, void*> context_;
};

struct HttpContext:Noncopyable{
    HttpParser* paser;
    ContextMap context;
};

namespace http{
void defaultHttpCallback(const HttpRequest* request, HttpResponse* response, ContextMap& context);
//void defaultHttpCallback(const HttpRequest* request, HttpResponse* response);
}

class HttpServer:Noncopyable{
public:
    typedef function<void(const HttpRequest* request, HttpResponse* response, ContextMap& context)> HttpCallback;
    //typedef function<void(const HttpRequest* request, HttpResponse* response)> HttpCallback;
    typedef TcpServer::ThreadInitCallback ThreadInitCallback;
    typedef function<void(bool connected, ContextMap& context)> ConnectionConnectCallback;
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
        assert(!server_.isStarted());
        httpCallback_ = cb;
    }

    void setConnectionConnectCallback(ConnectionConnectCallback cb){
        assert(!server_.isStarted());
        connConnectCallback_ = cb;
    }

    ~HttpServer(){}
private:
    void onConnect(const shared_ptr<TcpConnection>& conn){
        LOG_DEBUG << "on connect!";
        if(conn->isConnected()){
            HttpParser* paser = new HttpParser();
            HttpContext* httpContext = new HttpContext();
            httpContext->paser = paser;
            //conn->setContext(HttpParser());
            conn->setContext(httpContext);
            httpContext->context.setContext("CONNECTION", conn.get());//仅供测试？
            
            if(connConnectCallback_){
                connConnectCallback_(true, httpContext->context);
            }
        }
        else{
            HttpContext* httpContext = any_cast<HttpContext*>(conn->getContext());
            if(connConnectCallback_){
                connConnectCallback_(false, httpContext->context);
            }

            delete httpContext->paser;
            delete httpContext;
        }
    }
    
    void onMessage(const shared_ptr<TcpConnection>& conn, ConnBuffer* buffer, TimeStamp time){
        LOG_DEBUG << "on message!";
        conn->assertInIoLoop();
        HttpContext* httpContext = any_cast<HttpContext*>(conn->getContext());
        HttpParser* paser = httpContext->paser;
        
        bool toBeResolved = true;
        while(toBeResolved){
            if(paser->parseRequest(buffer, time)){//解析成功
                toBeResolved = false;
                if(paser->getState() == HttpParser::sGotAll){//一个请求解析完成
                    HttpResponse response;
                    const HttpRequest* request = paser->getRequest();
                    httpCallback_(request, &response, httpContext->context);//是不是应该把mysql设置成非阻塞的？如果mysql查询事件完成就保存结果、返还连接并且
                    conn->send(response.toString());

                    bool close = request->getHeaderValue("Connection") == "Close" || 
                                (request->getVersion() == http::Version::vHTTP1_0 && 
                                request->getHeaderValue("Connection") != "Keep-Alive");
                    if(close){
                        conn->shutdownWrite();
                    }

                    paser->reset();
                    if(buffer->readableBytes() > 0){//是否应该继续读取？两条请求可能同时发过来？
                        toBeResolved = true;
                    }
                }
            }else{//解析失败。
                toBeResolved = false;
                //关闭连接？还是先向回发送一个错误页面？
                //conn->forceClose();
                conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
                conn->shutdownWrite();
            }
        }
    }
    TcpServer server_;
    HttpCallback httpCallback_;
    ConnectionConnectCallback connConnectCallback_;
};
}

#endif