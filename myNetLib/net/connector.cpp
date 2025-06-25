#include "connector.h"

using namespace mynetlib;

    void Connector::restart(){
        loop_->assertInLoopThread();
        assert(tryingConnect_ == false);
        assert(state_ == sConnected);
        // if(state_ != sConnected){
        //     LOG_DEBUG<< "connector state:" << static_cast<int>(state_);
        // }
        LOG_DEBUG << "restart connector";
        tryingConnect_ = true;
        tryIntervalMs_ = 500;
        //当前fd已经交给TcpConnection管理了，TcpConnection析构时会关闭它.
        connectInLoop();//loop_->runInLoop(bind(&Connector::connectInLoop, this));
    }

    void Connector::startInLoop(){
        tryIntervalMs_ = 500;
        connectInLoop();
    }

    void Connector::log_hostAddress(int fd){
        InetAddress testAddr;
        sockets::getHostAddr(fd, testAddr.getAddr());
        LOG_DEBUG << "Try connect in addr " << testAddr.toString();
    }

    void Connector::connectInLoop(){
        loop_->assertInLoopThread();
        int fd = sockets::createNonblockingSocket(addr_.getFamily());
        int ret = ::connect(fd, (sockaddr*)addr_.getAddr(), sizeof(addr_));
        log_hostAddress(fd);
        LOG_TRACE << "Connector starts connect";
        int err = ret==0?0:errno;
        switch(err){
            case 0:
            case EINPROGRESS://正在等待建立连接
            case EINTR://只是connect被信号中断了，对于非阻塞socket而言，不一定代表连接失败了。
            case EISCONN://重复连接
                //LOG_DEBUG << "waitting for connecting";
                state_ = sConnecting;
                channel_.reset(new Channel(fd, loop_));
                channel_->setWritableCallback(bind(&Connector::handleWrite, this));
                channel_->setErrorCallback(bind(&Connector::handleError, this));
                channel_->enableWriting();//如果不放在loop中，可能丢失事件。
                break;
            case ENETUNREACH:
            case EADDRINUSE:
            case EAGAIN:
            case ECONNREFUSED://拒绝连接，可能服务还未启动
                retryInLoop();
                break;
            case EALREADY://当对一个非阻塞 socket 重复调用 connect()，而前一次连接尚未完成（无论是因为 EINPROGRESS 还是被信号中断的 EINTR）时，会返回 EALREADY。
                LOG_ERROR << "Already trying to connect: (" <<  err << ")" << strerror_tl(err);
                close(channel_->getFd());
                break;
            case ENOTSOCK://非socket
            case EBADF://文件描述符错误
            case EFAULT://内存错误
            case EAFNOSUPPORT://不支持协议族

            case EACCES://资源访问权限不足
            case EPERM://操作执行权限不足
                LOG_FATAL << "Unexpected error of connect: (" <<  err << ")" << strerror_tl(err);
                close(channel_->getFd());
                break;
            default:
                LOG_FATAL << "Unknow error of ::connect: (" <<  err << ")" << strerror_tl(err);
                close(channel_->getFd());
                break;
        }
    }

    void Connector::handleWrite(){
        loop_->assertInChannelHandling();
        assert(channel_);
        channel_->disableAll();
        channel_->remove();//在connectInLoop中reset
        
        if(sockets::getSocketError(channel_->getFd())!=0 || sockets::isSelfConnect(channel_->getFd())){
            retryInLoop();//继续尝试连接。
        }else{
            //成功建立连接，可能需要resetChannel，fd交给TcpConnection管理。（其实不resetChannel也没关系，可以等待析构时关闭或者restart时自动关闭）
            state_ = sConnected;
            tryingConnect_ = false;
            tryIntervalMs_ = 500;//一旦重新连接成功就重置。
            newConnectionCallback_(channel_->getFd());
        }
    }

    void Connector::handleError(){
        loop_->assertInChannelHandling();
        channel_->disableAll();
        channel_->remove();
        if(tryingConnect_){
            retryInLoop();
        }
    }

    void Connector::retryInLoop(){
        ::close(channel_->getFd());//立马关闭，防止后面又成功连接？
        state_ = sDisconnected;
        if(tryingConnect_){
            loop_->queueInLoop(bind(&Connector::resetChannel, this));//这个在接下来的pendingFunctor就会触发。

            TimeStamp tryStamp = freshRetryTime();
            auto timerId = loop_->runAt(tryStamp, bind(&Connector::tryConnectInLoop, shared_from_this()));//这个在下一次channel才会触发。
            assert(!retryTimerId_);
            retryTimerId_.reset(new TimerId(timerId));
        }
    }

    TimeStamp Connector::freshRetryTime(){
        TimeStamp tryStamp = TimeStamp::now();
        //随机初始间隔。
        if(tryIntervalMs_ <= 500){
            std::srand(static_cast<uint>(tryStamp.getMicroSecondsSinceEpoch()));
            tryIntervalMs_ = 500 + std::rand()%500;
        }
        tryStamp.add(tryIntervalMs_*1000);
        LOG_DEBUG << "Connecting failed. Retry in " << tryIntervalMs_/1000.0;
        tryIntervalMs_ = min(2*tryIntervalMs_, kMaxTryIntervalMs_);
        return tryStamp;
    }

    void Connector::resetChannel(){
        loop_->assertInPendingFunctors();
        channel_.reset();
    }

    void Connector::tryConnectInLoop(){
        LOG_DEBUG << "Start retry";
        loop_->assertInChannelHandling();
        resetTiemrId(false);//当前timer已经不存在了。
        if(!tryingConnect_ || state_!=sDisconnected){
            return;
        }
        connectInLoop();//loop_->runInLoop(bind(&Connector::connectInLoop, this));
    }