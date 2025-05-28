#ifndef WEBSERVER_NET_CONNBUFFER_H
#define WEBSERVER_NET_CONNBUFFER_H
#include <vector>
#include <assert.h>
#include <sys/uio.h>//readv、writev
#include <string>
namespace webserver{
    class ConnBuffer{
    public:
        ConnBuffer():readerIndex_(kPrependSize_), writerIndex_(kPrependSize_){
            buf_.resize(kPrependSize_+kInitialSize_);
            
        }
        ~ConnBuffer();

        

        char* begin(){return buf_.data();}
        char* readerBegin(){return buf_.data()+readerIndex_;}
        char* writerBegin(){return buf_.data()+writerIndex_;}
        size_t readableBytes(){return writerIndex_ - readerIndex_;}
        size_t writableBytes(){return buf_.size() - writerIndex_;}
        size_t prependableBytes(){return readerIndex_;}

        void hasWritten(size_t len){
            writerIndex_ += len;
        }
        void retrieveAll(){
            readerIndex_ = kPrependSize_;
            writerIndex_ = readerIndex_;
        }
        std::string retrieveAllAsString(){
            char* start = readerBegin();
            size_t len = readableBytes();
            retrieveAll();
            return std::string(start, len);
        }
        
        void prepend(char* data, size_t len){
            assert(readableBytes() >= len);
            std::copy(writerBegin()-len, writerBegin(), data);
            writerIndex_ -= len;
        }

        void append(const char* data, size_t len){
            if(writableBytes() < len){
                makeWritingSpace(len);
            }
            std::copy(data, data+len, writerBegin());
            hasWritten(len);
        }

        void makeWritingSpace(size_t len){
            size_t readable = readableBytes();
            if(prependableBytes() + writableBytes() >= len + kPrependSize_){//prependableBytes有可能大于kPrependSize_吗？为什么？因为按网络端读走数据了？貌似是的。
                std::copy(readerBegin(), writerBegin(), begin()+kPrependSize_);
                readerIndex_ = kPrependSize_;
                writerIndex_ = readerIndex_ + readable;
                assert(readable == readableBytes());
            }else{
                buf_.resize(kPrependSize_ + len + readable);//会自动复制数据到新内存。
            }
        }

        // void shinkToFit(){
        //     //到多大？
        // }

        void readFromFd(int fd){
            iovec vec[2];
            char buf[65535];
            size_t writable = writableBytes();
            vec[0].iov_base = writerBegin();
            vec[0].iov_len = writable;
            vec[1].iov_base = buf;
            vec[1].iov_len = sizeof buf;
            size_t n = static_cast<size_t>(readv(fd, vec, 2));
            
            if(n <= writable){
                hasWritten(n);
            }else{
                hasWritten(writable);
                size_t over = n - writable;
                makeWritingSpace(over);
                append(buf, sizeof(buf) - over);
            }
        }
    private:
        static int kPrependSize_;
        static int kInitialSize_;
        int readerIndex_;
        int writerIndex_;
        std::vector<char> buf_;

    };
}

#endif