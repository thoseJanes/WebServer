#include "fileUtil.h"

#include <unistd.h>
#include <iostream>
#include <fstream>

#include <assert.h>
#include <errno.h>
#include <sys/stat.h>

#include "../process/currentThread.h"
#include "../logging/logger.h"


namespace mynetlib{

namespace detail{
bool fileExists(string_view path) {
    std::ifstream file(path.data());
    return file.good();
}

int readToBuffer(int fd, char* buf, size_t maxSize, int* err){
    auto ret = read(fd, buf, maxSize);
    auto readLen = ret;
    while(readLen < maxSize){
        if(ret < 0){
            *err = errno;
            break;
        }else if(ret == 0){
            break;
        }
        ret = read(fd, buf+readLen, maxSize-readLen);
        readLen += ret;
    }

    return readLen;
}

}

int SmallFileReader::toBuffer(size_t maxSize){
    if(fd_< 0){
        return errno;
    }
    if(maxSize >= sizeof(buf_)){
        LOG_WARN << "Param maxSize exceeds avaliable buffer size " << sizeof(buf_)-1 << " will not working.";
    }

    int err = 0;
    struct stat statBuf;
    size_t len = min<size_t>(sizeof(buf_)-1, maxSize);
    if(fstat(fd_, &statBuf)){
        err = errno;
    }else{
        if(S_ISREG(statBuf.st_mode)){
            //len = min<int>(len, statBuf.st_size);//读取proc/self/stat时，大小为0？？
        }else if(S_ISDIR(statBuf.st_mode)){
            err = EISDIR;
        }
    }

    int rerr = continueReadOverwritesBuffer();
    return (rerr==0)?err:rerr;
}

int SmallFileReader::continueReadOverwritesBuffer(){
    int err = 0;
    size_t len = sizeof(buf_)-1;
    ssize_t readLen = detail::readToBuffer(fd_, buf_, len, &err);

    if(readLen >= len){
        assert(readLen == len);
        mayHaveMore_ = true;
        LOG_WARN << "The file may have been read only partially.";
    }else{
        mayHaveMore_ = false;
    }

    buf_[readLen] = '\0';
    len_ = readLen;

    return err;
}




int AdaptiveFileReader::toBuffer(size_t minBufferSize){
    if(fd_< 0){
        return errno;
    }
    auto err = 0;
    struct stat statBuf;
    
    if(fstat(fd_, &statBuf)){
        err = errno;
    }else{
        if(S_ISREG(statBuf.st_mode)){
            //len = min<int>(len, statBuf.st_size);//读取proc/self/stat时，大小为0？？
        }else if(S_ISDIR(statBuf.st_mode)){
            err = EISDIR;
        }
    }

    bufSize_ = max<size_t>(minBufferSize, statBuf.st_size+1);//为了防止触发mayHaveMore，+1。
    buf_ = static_cast<char*>(malloc(sizeof(char)*bufSize_+1));//留一个字节放末尾。
    bufInit_ = true;

    int rerr = continueReadOverwritesBuffer();
    return (rerr==0)?err:rerr;
}

int AdaptiveFileReader::continueReadOverwritesBuffer(){
    int err = 0;

    size_t len = bufSize_;
    ssize_t readLen = detail::readToBuffer(fd_, buf_, len, &err);

    if(readLen >= len){
        assert(readLen == len);
        mayHaveMore_ = true;
        LOG_WARN << "The file may have been read only partially.";
    }else{
        mayHaveMore_ = false;
    }

    buf_[readLen] = '\0';
    len_ = readLen;

    return err;
}







void FileAppender::append(const char* buf, const size_t len){
    size_t written = write(buf, len);
    while(written < len){
        auto err = ferror(file_);
        if(err){
            fprintf(stderr, "FileAppender::append() failed: %s\n", strerror_tl(err));
            break;
        }

        written += write(buf+written, len-written);
    }
}





}