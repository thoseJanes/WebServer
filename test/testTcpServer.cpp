#include "../net/tcpServer.h"
#include "../net/tcpClient.h"
#include <Types.h>

using namespace webserver;
void threadInitCallback(EventThread* ptr){
    LOG_INFO << "Event thread " << ptr->getName() <<" started";
}

void serverMessageCallback(const shared_ptr<TcpConnection>& conn, ConnBuffer* buf, TimeStamp time){
    string str = buf->retrieveAllAsString();
    if(str.size() < 10){
        conn->send(to_string(atoi(str.c_str())+1));
    }else{
        conn->send("0");
    }
}

void clientMessageCallback(const shared_ptr<TcpConnection>& conn, ConnBuffer* buf, TimeStamp time){
    string str = buf->retrieveAllAsString();
    if(str.size() < 10){
        conn->send(to_string(atoi(str.c_str())+1));
    }else{
        conn->send("0");
    }
}

void serverConnectCallback(const shared_ptr<TcpConnection>& conn){
    if(conn->isConnected()){
        LOG_INFO << "Connection from " << conn->peerAddressString();
    }else{
        LOG_INFO << "Connection from "<< conn->peerAddressString() << "closed";
    }
}

void clientConnectCallback(const shared_ptr<TcpConnection>& conn){
    if(conn->isConnected()){
        LOG_INFO << "Connect succeeded. Connection name "<< conn->nameString();
        conn->send("hello from " + conn->nameString());
    }else{
        LOG_INFO << "Connection "<< conn->nameString() << "closed";
    }
}

int main(){
    auto ret = fork();
    if(ret == 0){
        EventLoop* baseLoop = new EventLoop();
        InetAddress addr = InetAddress("127.0.0.1", 8887, AF_INET);
        TcpClient client = TcpClient(baseLoop, "testClient", addr);
        client.setConnectCallback(clientConnectCallback);
        client.setMessageCallback(clientMessageCallback);

        client.connect();
    }else{
        EventLoop* baseLoop = new EventLoop();
        InetAddress addr = InetAddress("127.0.0.1", 8080, AF_INET);
        TcpServer server = TcpServer(baseLoop, "testServer", addr, false);
        server.setConnectCallback(clientConnectCallback);
        server.setMessageCallback(clientMessageCallback);

        server.start(5, threadInitCallback);
    }




}