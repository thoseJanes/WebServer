#include "fileUtil.h"

#include "unistd.h"

#include <assert.h>
#include <errno.h>
#include <sys/stat.h>

#include "../thread/currentThread.h"

namespace webserver{

int SmallFileReader::toBuffer(int maxSize){
    if(fd_< 0){
        return errno;
    }

    auto err = errno;
    struct stat statBuf;
    int len = min<int>(sizeof(buf_)-1, maxSize);
    if(fstat(fd_, &statBuf)){
        err = errno;
    }else{
        if(S_ISREG(statBuf.st_mode)){
            len = min<int>(len, statBuf.st_size);
        }else if(S_ISDIR(statBuf.st_mode)){
            err = EISDIR;
        }
    }

    auto ret = read(fd_, buf_, len);
    auto readLen = ret;
    while(readLen < len){
        if(ret < 0){
            err = errno;
            break;
        }else if(ret == 0){
            break;
        }
        ret = read(fd_, buf_+readLen, len-readLen);
        readLen += ret;
    }
    buf_[readLen] = '\0';
    return err;
}

string_view SmallFileReader::toStringView(){
    return string_view(buf_, strlen(buf_));
}

void FileAppender::append(const char* buf, const size_t len){
    size_t written = write(buf, len);
    while(written < len){
        auto err = ferror(file_);
        if(err){
            fprintf(stderr, "FileAppender::append() failed: %s\n", CurrentThread::strerror_tl(err));
            break;
        }

        written += write(buf+written, len-written);
    }
}





}