#ifndef WEBSERVER_NET_CONNBUFFER_H
#define WEBSERVER_NET_CONNBUFFER_H
#include <vector>
#include <assert.h>
#include <sys/uio.h>//readv、writev
#include <string>
#include "../logging/logger.h"
namespace webserver{
    class ConnBuffer{
    public:
        ConnBuffer():readerIndex_(kPrependSize_), writerIndex_(kPrependSize_){
            buf_.resize(kPrependSize_+kInitialSize_);
            
        }
        ~ConnBuffer();

        const char* begin(){return buf_.data();}
        const char* readerBegin(){return buf_.data()+readerIndex_;}
        const char* writerBegin(){return buf_.data()+writerIndex_;}
        size_t readableBytes(){return writerIndex_ - readerIndex_;}
        size_t writableBytes(){return buf_.size() - writerIndex_;}
        size_t prependableBytes(){return readerIndex_;}

        const char* findCRLF(const char* start){
            assert(start < writerBegin());
            assert(start >= readerBegin());
            const char* pos = std::search(start, writerBegin(), kCRLF, kCRLF+2);
            return pos==writerBegin()?NULL:pos;
        }

        void hasWritten(size_t len){
            writerIndex_ += len;
        }
        void retrieveAll(){
            readerIndex_ = kPrependSize_;
            writerIndex_ = readerIndex_;
        }
        void retrieve(size_t n){
            if(n < readableBytes()){
                readerIndex_ += n;
            }else{
                retrieveAll();
            }
            
        }
        void retrieveTo(const char* pos){
            assert(pos >= readerBegin());
            assert(pos <= writerBegin());
            size_t offset = pos - readerBegin();
            retrieve(offset);
        }
        std::string retrieveAllAsString(){
            char* start = readerBegin();
            size_t len = readableBytes();
            retrieveAll();
            return std::string(start, len);
        }
        
        void prepend(char* data, size_t len){
            assert(prependableBytes() >= len);
            readerIndex_ -= len;
            std::copy(data, data+len, readerIndex_);
            
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

        ssize_t readFromFd(int fd){
            iovec vec[2];
            char buf[65535];
            size_t writable = writableBytes();
            vec[0].iov_base = writerBegin();
            vec[0].iov_len = writable;
            vec[1].iov_base = buf;
            vec[1].iov_len = sizeof buf;
            ssize_t ret = readv(fd, vec, 2);

            if(ret>0){
                size_t n = static_cast<size_t>(ret);
                if(n <= writable){
                    hasWritten(n);
                }else{
                    hasWritten(writable);
                    size_t over = n - writable;
                    makeWritingSpace(over);
                    append(buf, sizeof(buf) - over);
                }
            }
            return ret;
        }
    private:
        static int kPrependSize_;
        static int kInitialSize_;
        int readerIndex_;
        int writerIndex_;
        std::vector<char> buf_;
        static const char kCRLF[3];

    };
}

#endif