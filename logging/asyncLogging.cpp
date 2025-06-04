#include "asyncLogging.h"


namespace webserver{

AsyncLogging::AsyncLogging(string baseName, int rollSize, int flushInterval):
    flushInterval_(flushInterval), output_(baseName, rollSize, 1024, flushInterval, false),
    mutex_(), notEmpty_(mutex_), running_(false),
    currentBuffer_(new Buffer), nextBuffer_(new Buffer),
    thread_(bind(&AsyncLogging::loggingThreadFunc, this), "logging"){}

void AsyncLogging::append(const char* logline, int len){
    if(running_){//问题：running_判断需要上锁吗？布尔值的赋值操作是否可以近似认为是原子操作？
        MutexLockGuard lock(mutex_);
        if(currentBuffer_->availBytes() <= len){
            buffers_.emplace_back(std::move(currentBuffer_));
            notEmpty_.notify();
            if(nextBuffer_){
                currentBuffer_.swap(nextBuffer_);
            }else{
                currentBuffer_.reset(new Buffer);
            }
        }
        assert(currentBuffer_->availBytes() > len);
        currentBuffer_->append(logline, len);//假设len小于最大长度？
    }
}

void AsyncLogging::stop(){
    if(running_){
        running_ = false;
        notEmpty_.notify();//直接唤醒线程。只会有一个线程等待在该条件量上。
        thread_.join();//等待线程结束回收资源
    }
}

void AsyncLogging::start(){
    if(!running_){
        running_ = true;
        thread_.start();
    }
}

void AsyncLogging::loggingThreadFunc(){
    vector<unique_ptr<Buffer>> buffers;
    buffers.reserve(16);
    unique_ptr<Buffer> buffer1(new Buffer);
    unique_ptr<Buffer> buffer2(new Buffer);//问题：智能指针的赋值方式
    while(running_){
        {
            MutexLockGuard lock(mutex_);
            while(buffers_.empty()){
                notEmpty_.waitSeconds(flushInterval_);
                buffers_.push_back(std::move(currentBuffer_));
                if(buffer1){//在这里buffer1是否有可能为空？貌似并不可能
                    currentBuffer_.swap(buffer1);
                }
                if(!nextBuffer_){
                    nextBuffer_.swap(buffer2);
                }
            }
            buffers.swap(buffers_);
        }

        if(buffers.size() > 25){
            char buf[128];
            int len = snprintf(buf, sizeof buf, "Discard %ld buffers.\n", buffers.size()-2);
            output_.append(buf, len);
            buffers.resize(2);//这里抛弃的buffer是最新的buffer？（为了防止无效输出淹没真正错误原因）
        }

        for(unique_ptr<Buffer>& buffer:buffers){
            output_.append(buffer->data(), buffer->writtenBytes());
        }

        if(buffers.size() > 2){
            buffers.erase(buffers.begin(), buffers.end()-2);
        }
        
        if(!buffer1){
            buffer1.swap(buffers.back());
            buffer1->reset();
            buffers.pop_back();
        }
        if(!buffer2){
            buffer2.swap(buffers.back());
            buffer2->reset();
            buffers.pop_back();
        }
        buffers.clear();
        output_.flush();
    }
    output_.flush();
}


AsyncLogging::~AsyncLogging(){
    if(running_){
        stop();
    }
}

}