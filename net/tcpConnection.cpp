#include "tcpConnection.h"


using namespace webserver;


void detail::defaultMessageCallback(const shared_ptr<TcpConnection>& conn, ConnBuffer* buffer, TimeStamp timeStamp){
    LOG_INFO << "Receive message \"" << buffer->retrieveAllAsString() << "\" at time " << timeStamp.toFormattedString(false);
    //buffer->retrieveAll();
}

void detail::defaultConnectCallback(const shared_ptr<TcpConnection>& conn){
    LOG_INFO << "Connection established between host "<< conn->hostAddressString() << ", peer " << conn->peerAddressString();
}

TcpConnection::TcpConnection(int fd, string_view name, EventLoop* loop)
:   loop_(loop), 
    channel_(new Channel(fd, loop)),
    socket_(fd),
    name_(name),
    hostAddr_(socket_.getHostAddr()),
    peerAddr_(socket_.getPeerAddr()),
    state_({sConnecting, false})
{
    
    //channel在执行回调时，TcpConnection必须存在。
    channel_->setCloseCallback(bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(bind(&TcpConnection::handleError, this));
    channel_->setReadableCallback(bind(&TcpConnection::handleRead, this));
    channel_->setWritableCallback(bind(&TcpConnection::handleWrite, this));
    socket_.setKeepAlive(true);
}

TcpConnection::~TcpConnection(){
    LOG_DEBUG << "connection " << name_ << " dtor()";
}

void TcpConnection::connectEstablished(){
    assert(state_.connState == sConnecting);
    channel_->tie(shared_from_this());//注意，这里只有用void类型的指针才能共享控制块，见smartPtrTest
    channel_->enableReading();

    state_.setConnectionState(sConnected);//应当在connected之前设置好回调函数？或者用锁保护回调函数？
    if(connectCallback_){
        connectCallback_(shared_from_this());
    }
}

void TcpConnection::setTcpNoDelay(bool bl){
    socket_.setTcpNoDelay(bl);
}

void TcpConnection::send(const void* data, size_t len){
    send(string_view(static_cast<const char*>(data), len));
}
void TcpConnection::send(string_view str){
    if(loop_->isInLoopThread()){
        sendInLoop(str.data(), str.size());
    }else{
        loop_->runInLoop(bind<void(TcpConnection::*)(string&)>(&TcpConnection::sendInLoop, this, string(str)));
    }
}
void TcpConnection::send(ConnBuffer* buffer){
    if(loop_->isInLoopThread()){
        sendInLoop(buffer->readerBegin(), buffer->readableBytes());
        buffer->retrieveAll();
    }else{
        loop_->runInLoop(bind<void(TcpConnection::*)(string&)>(&TcpConnection::sendInLoop, this, buffer->retrieveAllAsString()));
    }
}
void TcpConnection::sendDelay(string_view str, int delayMs){
    loop_->runAfter(delayMs, bind<void(TcpConnection::*)(string&)>(&TcpConnection::sendInLoop, this, string(str)));
}
void TcpConnection::sendInLoop(string& str){
    sendInLoop(str.data(), str.size());
}
void TcpConnection::sendInLoop(const void* data, size_t len){
    assert(state_.connState == sConnected);
    // if(state_.shutdown){
    //     return;
    // }
    loop_->assertInLoopThread();
    size_t remain = len;
    if(!channel_->isWritingEnabled() && outputBuffer_.readableBytes() == 0){
        ssize_t sent = socket_.write(static_cast<const char*>(data), len);
        if(sent >= 0){
            remain -= static_cast<size_t>(sent);
            if(remain == 0 && writeCompleteCallback_){
                writeCompleteCallback_(shared_from_this());
            }
        }else{
            LOG_SYSERROR << "Failed in TcpConnection::sendInLoop.";
            handleError();
        }
    }
    //什么情况下可以直接发送数据?
    if(remain > 0){
        auto size = outputBuffer_.readableBytes();
        outputBuffer_.append(static_cast<const char*>(data) + len - remain, remain);
        assert(outputBuffer_.readableBytes() == size + remain);
        if(size+remain >= highWaterBytes_ && size < highWaterBytes_ && highWaterCallback_){
            highWaterCallback_(shared_from_this(), size+remain);
        }

        if(!channel_->isWritingEnabled() && state_.connState == sConnected){
            channel_->enableWriting();
            //state_.isWriting = true;
        }
    }
}

void TcpConnection::startRead(){
    assert(state_.connState == sConnected);
    state_.isReading = true;
    loop_->runInLoop(bind(&TcpConnection::startReadInLoop, this));
}
void TcpConnection::startReadInLoop(){
    loop_->assertInLoopThread();
    if(state_.isReading && !channel_->isReadingEnabled()){
        channel_->enableReading();
    }
}
void TcpConnection::stopRead(){
    state_.isReading = false;
    loop_->runInLoop(bind(&TcpConnection::stopInReadLoop, this));
}
void TcpConnection::stopInReadLoop(){
    loop_->assertInLoopThread();
    if(!state_.isReading && channel_->isReadingEnabled()){
        channel_->disableReading();
    }
}


void TcpConnection::handleWrite(){
    loop_->assertInChannelHandling();
    if(!channel_->isWritingEnabled()){//事件处理顺序：close->error->read->write，防止连接已经准备被read关闭
        return;
    }
    auto remain = outputBuffer_.readableBytes();
    if(remain > 0){
        ssize_t n = socket_.write(outputBuffer_.readerBegin(), outputBuffer_.readableBytes());
        if(n >= 0){
            outputBuffer_.retrieve(n);
            remain -= static_cast<size_t>(n);
            if(remain == 0){
                channel_->disableWriting();
                if(writeCompleteCallback_){
                    writeCompleteCallback_(shared_from_this());
                }

                if(state_.connState == sDisConnecting){
                    shutdownWriteInLoop();
                }
            }
            
        }else{
            LOG_SYSERROR << "Failed in TcpConnection::handleWrite()." ;
            handleError();
        }
    }else{
        LOG_ERROR << "Handling write when output buffer remaining equals zero.";
    }
}
void TcpConnection::handleRead(){
    loop_->assertInChannelHandling();
    ssize_t ret = inputBuffer_.readFromFd(channel_->getFd());
    if(ret < 0){
        LOG_SYSERROR << "Failed in TcpConnection::handleRead() when calling readFromFd().";
        handleError();
    }else if(ret == 0){//表示对端已经关闭了连接。
        LOG_ERROR << "read zero from "<< name_;
        handleClose();
    }else{
        if(messageCallback_){
            messageCallback_(shared_from_this(), &inputBuffer_, TimeStamp::now());
        }
    }
    //inputBuffer_.retrieveAll();???是否需要？
}
void TcpConnection::handleError(){
    int err = sockets::getSocketError(socket_.fd());
    LOG_ERROR << "Connection: " << name_ << ". Error(" << errno << ") from errno:" << strerror_tl(errno) <<
            ". Error(" << err << ") from socket:" << strerror_tl(err)
             ;//<< "\n" << "backTrace:" << CurrentThread::backTraceStacks(true);
}
void TcpConnection::handleClose(){
    LOG_ERROR << "start handle close from "<< name_;
    state_.setConnectionState(sDisConnecting);
    loop_->queueInLoop(bind(&TcpConnection::closeInLoop, this));//能保证加在目前的最后面执行，且可以安全地操作channel。
}


void TcpConnection::forceClose(){
    state_.setConnectionState(sDisConnecting);
    loop_->queueInLoop(bind(&TcpConnection::closeInLoop, shared_from_this()));//能保证加在目前的最后面执行，且可以安全地操作channel。
}
void TcpConnection::closeInLoop(){//如果close最后执行了，这个函数在loop中一定是tcpConnection最后执行的?
    LOG_DEBUG << "closeInLoop from connection:" << name_;
    loop_->assertInPendingFunctors();

    channel_->disableAll();
    channel_->remove();

    state_.setConnectionState(sDisConnected);
    if(connectCallback_){
        connectCallback_(shared_from_this());
    }
    if(closeCallback_){
        closeCallback_(shared_from_this());
    }
}

void TcpConnection::shutdownWrite(){
    LOG_DEBUG << "shutdown in connection "<<name_;
    state_.setConnectionState(sDisConnecting);
    if(!channel_->isWritingEnabled()){
        loop_->queueInLoop(bind(&TcpConnection::shutdownWriteInLoop, this));
    }
}
void TcpConnection::shutdownWriteInLoop(){
    socket_.shutDownWrite();
}
