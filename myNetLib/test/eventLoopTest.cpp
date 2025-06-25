
#include "../event/eventLoop.h"
#include "../event/eventThread.h"
#include "../event/eventThreadPool.h"
#include "../net/socket.h"
#define MYNETLIB_LOG_TRACE
using namespace mynetlib;
void timerCallback(int i){
    TimeStamp now = TimeStamp::now();
    printf("%d\n", i);
    LOG_INFO << "timer " << i << " run at " << now.toFormattedString(true);//??int为0时匹配不到吗？
    //sleep(1);
}

void writableCallback(){
    TimeStamp now = TimeStamp::now();
    LOG_INFO << " writable at " << now.toFormattedString(true);
    sleep(1);
}

void threadInitCallback(EventThread* thread){
    LOG_INFO << "thread "<< thread->getName() <<" init at "<<TimeStamp::now().toFormattedString(true);
}


std::atomic<int> cbId(0);
int main(){
    Global::setGlobalLogLevel(Logger::LogLevel::Debug);
    LOG_TRACE << "trace" << 0  << -1;
    LOG_DEBUG << "debug" << 0  << -1;
    LOG_INFO << "info" << 0  << -1;
    auto pid = fork();
    if(pid == 0){//子进程
        sleep(4);
        Socket socket(sockets::createNonblockingSocket(AF_INET));
        InetAddress addr("127.0.0.1", 8080, AF_INET);
        connect(socket.fd(), reinterpret_cast<sockaddr*>(&(addr.getAddr()->addr6)), sizeof(addr));
    }else{//父进程
        //EventThread* baseThread = new EventThread("baseThread");
        EventLoop* baseLoop = new EventLoop();
        unique_ptr<EventThreadPool> threadPool(new EventThreadPool(baseLoop, "testPool"));
        
        threadPool->start(5, threadInitCallback);
        
        EventLoop* loop = threadPool->getNextLoop();
        
        Socket socket(sockets::createNonblockingSocket(AF_INET));
        InetAddress addr("192.168.29.61", 8080, AF_INET);
        
        // sockaddr_in addr;
        // memset(&addr, 0, sizeof(addr));
        // addr.sin_port = htons(8080);
        // inet_pton(AF_INET, "127.0.0.1",&addr.sin_addr);
        // addr.sin_family = AF_INET;
        // if(::bind(socket.fd(), (sockaddr*)&(addr), sizeof(addr)) < 0){
        //     LOG_SYSFATAL << "bind failed";
        // }
        // if(::bind(socket.fd(), (sockaddr*)&(addr.getAddr()->addr6), sizeof(addr.getAddr()->addr)) < 0){
        //     LOG_SYSFATAL << "bind failed, family:" << addr.getFamily() << " ip:" << addr.toString();
        // }

        socket.bind(addr);
        socket.listen();
        Channel channel(socket.fd(), loop);

        channel.setWritableCallback(writableCallback);
        channel.enableWriting();
        loop->queueInLoop(bind(timerCallback, cbId.fetch_add(1)));
        loop->runAfter(1500, bind(timerCallback, cbId.fetch_add(1)));
        loop->runAfter(999*2, bind(timerCallback, cbId.fetch_add(1)));
        loop->runAfter(1000*2, bind(timerCallback, cbId.fetch_add(1)));
        loop->runAfter(1001*2, bind(timerCallback, cbId.fetch_add(1)));
        baseLoop->loop();


    }
    
    
    

    return 0;
}