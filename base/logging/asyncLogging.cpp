#include "asyncLogging.h"


namespace webserver{

AsyncLogging::AsyncLogging(string baseName, int flushInterval):
    flushInterval_(flushInterval), output_(baseName),
    mutex_(), notEmpty_(mutex_){

    currentBuffer_.reset(new Buffer);
    nextBuffer_.reset(new Buffer);
    thread_.setName("logging");
    thread_.start(bind(loggingThreadFunc, this));
}

void AsyncLogging::append(const char* logline, int len){
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

void AsyncLogging::loggingThreadFunc(){
    vector<unique_ptr<Buffer>> buffers;
    {
        MutexLockGuard lock(mutex_);
        while(buffers_.size()==0){
            notEmpty_.waitSeconds(flushInterval_);
            buffers_.push_back(std::move(currentBuffer_));
            currentBuffer_.reset(new Buffer);
        }
        buffers.swap(buffers_);
        
    }

    if(buffers.size() > 25){
        buffers.resize(2);
    }

    for(unique_ptr<Buffer>& buffer:buffers){
        output_.append(buffer->data(), buffer->writtenBytes());
    }

    if(buffers.size() > 2){
        buffers.erase(buffers.begin(), buffers.end()-2);
    }
    
    {
        MutexLockGuard lock(mutex_);
        if(!nextBuffer_){
            nextBuffer_.swap(buffers.back());
            nextBuffer_->reset();
            buffers.pop_back();
        }
        if(!currentBuffer_){
            nextBuffer_.swap(buffers.back());
            nextBuffer_->reset();
            buffers.pop_back();
        }
    }
}


}