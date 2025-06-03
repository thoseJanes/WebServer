#include "../net/tcpServer.h"
#include "../net/tcpClient.h"

using namespace webserver;
void threadInitCallback(EventThread* ptr){
    LOG_INFO << "Event thread " << ptr->getName() <<" started";
}

void serverMessageCallback(const shared_ptr<TcpConnection>& conn, ConnBuffer* buf, TimeStamp time){
    string str = buf->retrieveAllAsString();
    LOG_INFO << "server [" << conn->nameString() << "] received:" << str;
    if(str.size() < 10){
        conn->sendDelay(to_string(atoi(str.c_str())+1), 500);
    }else{
        conn->send("0");
    }
}

void clientMessageCallback(const shared_ptr<TcpConnection>& conn, ConnBuffer* buf, TimeStamp time){
    string str = buf->retrieveAllAsString();
    LOG_INFO << "client [" << conn->nameString() << "] received:" << str;
    if(str.size() < 10){
        int count = atoi(str.c_str())+1;
        if(count > 10){
            conn->forceClose();
        }else{
            conn->sendDelay(to_string(count), 2000);
        }
    }else{
        conn->send("0");
    }
}

void serverConnectCallback(const shared_ptr<TcpConnection>& conn){
    if(conn->isConnected()){
        LOG_INFO << "Connection from " << conn->peerAddressString();
    }else{
        LOG_INFO << "Connection ["<< conn->nameString() << "] closed";
    }
}

void clientConnectCallback(const shared_ptr<TcpConnection>& conn){
    if(conn->isConnected()){
        LOG_INFO << "Connect succeeded. Connection name "<< conn->nameString();
        //conn->closeInLoop
        conn->send("hello from " + conn->nameString());
    }else{
        LOG_INFO << "Connection ["<< conn->nameString() << "] closed";
    }
}

void writeCompleteCallback(const shared_ptr<TcpConnection>& conn){
    LOG_INFO << "send over. Connection name "<< conn->nameString();
}


int main(){
    Global::setGlobalLogLevel(Logger::Debug);
    int connectNum = 2;
    int forkTimes = 0;
    while(connectNum >> (forkTimes+1) > 0){
        forkTimes ++;
    }
    LOG_INFO << "forkTimes: "<<forkTimes;
    auto ret = fork();
    if(ret == 0){
        for(int i=0;i<forkTimes;i++){
            fork();
        }
        EventLoop* baseLoop = new EventLoop();
        InetAddress addr = InetAddress("127.0.0.1", 8081, AF_INET);
        TcpClient client = TcpClient(baseLoop, "testClient"+to_string(CurrentThread::tid()), addr);
        client.setConnectCallback(clientConnectCallback);
        client.setMessageCallback(clientMessageCallback);
        client.setWriteCompleteCallback(writeCompleteCallback);

        client.connect();
        baseLoop->loop();
        //baseLoop->runAfter(5000, TcpClient::)
    }else{
        EventLoop* baseLoop = new EventLoop();
        InetAddress addr = InetAddress("127.0.0.1", 8081, AF_INET);
        TcpServer server = TcpServer(baseLoop, "testServer", addr, false);
        server.setConnectCallback(serverConnectCallback);
        server.setMessageCallback(serverMessageCallback);
        server.setWriteCompleteCallback(writeCompleteCallback);

        server.start(2, threadInitCallback);
        baseLoop->loop();
    }




}