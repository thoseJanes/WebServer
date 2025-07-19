#ifndef MYNETLIB_NET_CONNBUFFER_H
#define MYNETLIB_NET_CONNBUFFER_H
#include <cstdint>
#include <endian.h>
#include <vector>
#include <assert.h>
#include <sys/uio.h>//readv、writev
#include <string>
#include "../../mynetbase/logging/logger.h"
#include "endian.h"
namespace mynetlib{
    //注意：不论如何，prepend的数据都是在prependSize处结束的。
    //也就是说，prepend的数据和append的数据是可以区分开来的。（除非unwrite太多了,或者先retrieve再prepend）
    class ConnBuffer{
    public:
        ConnBuffer(size_t initialSize = 1024, size_t prependSize = 8):readerIndex_(prependSize), writerIndex_(prependSize), prependSize_(prependSize), initialSize_(initialSize){
            buf_.resize(prependSize+initialSize);
        }
        ~ConnBuffer(){}

        char* begin(){return buf_.data();}
        char* readerBegin(){return buf_.data()+readerIndex_;}
        char* writerBegin(){return buf_.data()+writerIndex_;}
        const char* begin() const {return buf_.data();}
        const char* readerBegin() const {return buf_.data()+readerIndex_;}
        const char* writerBegin() const {return buf_.data()+writerIndex_;}
        size_t readableBytes() const {return writerIndex_ - readerIndex_;}
        size_t writableBytes() const {return buf_.size() - writerIndex_;}
        size_t prependableBytes() const {return readerIndex_;}
        //size_t prependedBytes(){return }

        const char* findCRLF(const char* start){
            assert(start < writerBegin());
            assert(start >= readerBegin());
            const char* wb = writerBegin();
            const char* pos = std::search(start, wb, kCRLF, kCRLF+2);
            return pos==writerBegin()?NULL:pos;
        }

        void hasWritten(size_t len){
            writerIndex_ += len;
        }
        void unwrite(size_t len){
            assert(len <= readableBytes());
            writerIndex_ -= len;
        }
        void retrieveAll(){
            readerIndex_ = prependSize_;
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
            const char* start = readerBegin();
            size_t len = readableBytes();
            retrieveAll();
            return std::string(start, len);
        }
        std::string toString() const {
            const char* start = readerBegin();
            size_t len = readableBytes();
            return std::string(start, len);
        }
        

        void readTo(ConnBuffer buf, size_t n){
            assert(n <= readableBytes());
            buf.append(readerBegin(), n);
            retrieve(n);
        }

        void read(void* buf, size_t n){
            memcpy(buf, readerBegin(), n);
            retrieve(n);
        }

        uint8_t readByte(){
            uint8_t val;
            memcpy(&val, readerBegin(), 1);
            retrieve(1);
            return val;
        }

        #define READ_NETWORK_UINT_GENERATOR(uintlen)\
        uint ## uintlen ## _t read ## Uint ## uintlen(){\
            uint ## uintlen ## _t val;\
            read(&val, sizeof(val));\
            val = networkToHost ## uintlen(val);\
            return val;\
        }

        READ_NETWORK_UINT_GENERATOR(16);
        READ_NETWORK_UINT_GENERATOR(32);
        READ_NETWORK_UINT_GENERATOR(64);
        #undef INSERT_NETWORK_UINT_GENERATOR

        void prepend(const void* data, size_t len){
            assert(prependableBytes() >= len);
            readerIndex_ -= len;
            const char* d = static_cast<const char*>(data);
            std::copy(d, d+len, readerBegin());
        }

        void append(const void* data, size_t len){
            if(writableBytes() < len){
                makeWritingSpace(len);
            }
            const char* d = static_cast<const char*>(data);
            std::copy(d, d+len, writerBegin());
            hasWritten(len);
        }

        #define INSERT_NETWORK_UINT_GENERATOR(method, uintlen)\
        void method ## Uint ## uintlen(uint ## uintlen ## _t val){\
            val = hostToNetwork ## uintlen(val);\
            method(&val, sizeof(val));\
        }

        INSERT_NETWORK_UINT_GENERATOR(append, 16);
        INSERT_NETWORK_UINT_GENERATOR(append, 32);
        INSERT_NETWORK_UINT_GENERATOR(append, 64);
        INSERT_NETWORK_UINT_GENERATOR(prepend, 16);
        INSERT_NETWORK_UINT_GENERATOR(prepend, 32);
        INSERT_NETWORK_UINT_GENERATOR(prepend, 64);
        #undef INSERT_NETWORK_UINT_GENERATOR


        void makeWritingSpace(size_t len){
            size_t readable = readableBytes();
            if(prependableBytes() + writableBytes() >= len + prependSize_){//prependableBytes有可能大于kPrependSize_吗？为什么？因为按网络端读走数据了？貌似是的。
                std::copy(readerBegin(), writerBegin(), begin()+prependSize_);
                readerIndex_ = prependSize_;
                writerIndex_ = readerIndex_ + readable;
                assert(readable == readableBytes());
            }else{
                buf_.resize(prependSize_ + len + readable);//会自动复制数据到新内存。
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
                    append(buf, over);
                }
            }
            return ret;
        }
    private:
        // static int kPrependSize_;
        // static int kInitialSize_;
        size_t prependSize_;
        size_t initialSize_;
        size_t readerIndex_;
        size_t writerIndex_;
        std::vector<char> buf_;
        //static const char kCRLF[3];
        static constexpr char kCRLF[3] = "\r\n";
    };
}

#endif