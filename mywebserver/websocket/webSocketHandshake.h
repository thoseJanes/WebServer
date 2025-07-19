#ifndef MYWEBSERVER_WEBSOCKET_WEBSOCKETHANDSHAKE_H
#define MYWEBSERVER_WEBSOCKET_WEBSOCKETHANDSHAKE_H
#include "webSocket.h"
#include "../http/httpResponse.h"
namespace mywebserver{

class ClientWebSocketHandshakeContext{
public:
    ClientWebSocketHandshakeContext():request_(TimeStamp::now(), http::mGET, http::vHTTP1_1), isKeySet_(false){
        request_.setHeaderValue("Upgrade", "websocket");
        request_.setHeaderValue("Connection", "Upgrade");
        request_.setHeaderValue("Sec-WebSocket-Version", "13");
    }

    void generateRequestKey(){
        unsigned char orgRandKey[webSocket::kOrgKeyLength_];
        webSocket::generateRandBytes(orgRandKey, webSocket::kOrgKeyLength_);

        size_t len = webSocket::encodeBase64(orgRandKey, webSocket::kOrgKeyLength_, 
                                            webSocketKey_, sizeof(webSocketKey_));
        assert(len == sizeof(webSocketKey_));
        request_.setHeaderValue("Sec-WebSocket-Key", string(webSocketKey_, webSocket::kKeyLength_));
        isKeySet_ = true;
    }
    string getRequestKey(){
        if(isKeySet_){
            return string(webSocketKey_, webSocket::kKeyLength_);
            //memcpy(key, webSocketKey_, webSocket::kKeyLength_);
        }else{
            LOG_ERROR << "WebSocketHandshakeRequest::getKey - key has not been set";
            return "";
        }
    }
    void setRequestProtocol(const string& str){
        request_.setHeaderValue("Sec-WebSocket-Protocol", str);
    }
    void setRequestExtensions(const string& str){
        request_.setHeaderValue("Sec-WebSocket-Extensions", str);
    }

    std::string getRequestString(){
        if(!isKeySet_){
            generateRequestKey();
        }
        return request_.toString();
    }
    const HttpRequest& getHttpRequest(){
        if(!isKeySet_){
            generateRequestKey();
        }
        return request_;
    }

    bool isResponseValid(const HttpResponse& response){
        return webSocket::isWebSocketHandshakeResponseValid(request_, response);
    }
private:
    // void formatKey(char* buf){
    //     char* pos = buf;
    //     for(int i=0;i<webSocket::kKeyLength_;i++){
    //         size_t ret = detail::formatInteger<unsigned char>(pos, webSocketKey_[i]);
    //         assert(ret == 1);
    //         pos++;
    //     }
    // }
    char webSocketKey_[webSocket::kKeyLength_];//即16除3向上取整再乘4.
    bool isKeySet_;
private:
    HttpRequest request_;
};

class WebSocketHandshakeResponse{
public:
    explicit WebSocketHandshakeResponse(const HttpRequest& request):response_(&request){
        assert(webSocket::isWebSocketHandshakeRequestValid(request));
        response_.setStatusCode(101);
        response_.setHeaderValue("Upgrade", "websocket");
        response_.setHeaderValue("Connection", "Upgrade");
        response_.setHeaderValue("Sec-WebSocket-Version", request.getHeaderValue("Sec-WebSocket-Version"));

        string acceptKey = webSocket::calAcceptKey(request.getHeaderValue("Sec-WebSocket-Key"));
        response_.setHeaderValue("Sec-WebSocket-Accept", acceptKey);
        //处理扩展和协议。
        handleRequestProtocol(request);
        handleRequestExtensions(request);
    }

    void handleRequestProtocol(const HttpRequest& request){
        //TOBECOMPLETED
    }
    void handleRequestExtensions(const HttpRequest& request){
        //TOBECOMPLETED
    }

    string toString(){
        return response_.toString();
    }
private:
    HttpResponse response_;
};


}


#endif// MYWEBSERVER_WEBSOCKET_WEBSOCKETHANDSHAKE_H