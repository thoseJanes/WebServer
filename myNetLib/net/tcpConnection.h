#ifndef MYNETLIB_NET_TCPCONNECTION_H
#define MYNETLIB_NET_TCPCONNECTION_H

#include "inetAddress.h"
#include "connBuffer.h"
#include "socket.h"
#include "../event/channel.h"
#include "../event/eventLoop.h"
#include "../process/currentThread.h"
#include <functional>
#include <memory>

namespace mynetlib{
class TcpConnection;
typedef std::function<void(const shared_ptr<TcpConnection>&, ConnBuffer* buffer, TimeStamp timeStamp)> MessageCallback;
typedef std::function<void(const shared_ptr<TcpConnection>&)> ConnectCallback;
typedef std::function<void(const shared_ptr<TcpConnection>&)> WriteCompleteCallback;
typedef std::function<void(const shared_ptr<TcpConnection>&, size_t)> HighWaterCallback;
typedef std::function<void(const shared_ptr<TcpConnection>&)> CloseCallback;
namespace detail{
    void defaultMessageCallback(const shared_ptr<TcpConnection>& conn, ConnBuffer* buffer, TimeStamp timeStamp);

    void defaultConnectCallback(const shared_ptr<TcpConnection>& conn);
}




//用法：[setHighWaterCallback]->控制send或者read
class TcpConnection : Noncopyable, public enable_shared_from_this<TcpConnection>{
public:

    typedef ConnBuffer Buffer;

    enum ConnState{
        sDisConnected,
        sDisConnecting,
        sConnected,
        sConnecting,
    };
    struct State{
        ConnState connState;
        bool isReading;
        //size_t bytesToBeSent;
        void setConnectionState(ConnState state){
            connState = state;
        }
    };

    TcpConnection(int fd, string_view name, EventLoop* loop);
    ~TcpConnection();
    void connectEstablished();

    void setHighWaterCallback(size_t highWaterBytes, HighWaterCallback cb){
        highWaterBytes_ = highWaterBytes;
        highWaterCallback_ = cb;
    }
    void setWriteCompleteCallback(WriteCompleteCallback cb){
        writeCompleteCallback_ = cb;
    }
    void setCloseCallback(CloseCallback cb){
        closeCallback_ = cb;
    }
    void setMessageCallback(MessageCallback cb){
        messageCallback_ = cb;
    }
    void setConnectCallback(ConnectCallback cb){
        connectCallback_ = cb;
    }

    void assertInIoLoop(){
        loop_->assertInLoopThread();
    }

    void setTcpNoDelay(bool bl);

    void send(const void* buf, size_t len);
    void send(string_view str);
    void send(const ConnBuffer* buffer);
    void sendAndRetrieve(ConnBuffer* buffer);
    void sendDelay(string_view str, int delayMs);

    void startRead();
    void stopRead();


    void forceClose();
    

    void shutdownWrite();

    

    string hostAddressString() const {
        return hostAddr_.toString();
    }
    string peerAddressString() const {
        return peerAddr_.toString();
    }
    string nameString() const {
        return name_;
    }
    
    bool isConnected() const {
        return state_.connState == sConnected;
    }

    // void setContext(string key, void* context) const {
    //     assert(!hasContext(key));
    //     context_.insert({key, context});
    // }
    // bool hasContext(string key) const {
    //     return context_.find(key) != context_.end();
    // }
    // void* getContext(string key) const {
    //     return context_.at(key);
    // }
    // void removeContext(string key) const{
    //     auto ret = context_.erase(key);
    //     assert(ret == 1);
    // }


    void setContext(const std::any& context) {
        context_ = context;
    }
    
    std::any getContext() {
        return context_;
    }

private:
    void shutdownWriteInLoop();

    void handleWrite();
    void handleRead();
    void handleError();
    void handleClose();
    void closeInLoop();

    void startReadInLoop();
    void stopInReadLoop();

    void sendInLoop(string& str);
    void sendInLoop(const void* data, size_t len);
    State state_;
    EventLoop* loop_;
    unique_ptr<Channel> channel_;
    Socket socket_;
    const InetAddress hostAddr_;
    const InetAddress peerAddr_;
    ConnBuffer inputBuffer_;
    ConnBuffer outputBuffer_;

    MessageCallback messageCallback_;//读到数据
    ConnectCallback connectCallback_;
    WriteCompleteCallback writeCompleteCallback_;//写完数据
    HighWaterCallback highWaterCallback_;
    CloseCallback closeCallback_;

    size_t highWaterBytes_;
    string name_;

    std::any context_;
};

}


#endif