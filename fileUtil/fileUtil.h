#ifndef WEBSERVER_FILEUTIL_FILEUTIL_H
#define WEBSERVER_FILEUTIL_FILEUTIL_H


#include <stdio.h>
#include <sys/uio.h>
#include <fcntl.h>
#include "./common/stringArg.h"

namespace webserver{


class SmallFileReader{
public:
    //SmallFileReader(StringArg path):fd_(open(path.c_str(), O_CLOEXEC|O_RDONLY)){}
    //void toBuffer(char* buf, int maxLen);
    SmallFileReader(StringArg path):fd_(open(path.c_str(), O_CLOEXEC|O_RDONLY)){}
    void toStringView();
private:
    void toBuffer();
    int fd_;
    const char* buffer[64*1024];
};



class FileAppender{
public:
    FileAppender(StringArg path):file_(fopen(path.c_str(), "ea")){
        setbuffer(file_, buf_, sizeof buf_);
    }
    void append(const char* buf, const size_t len);//是否用string_view？
    size_t write(const char* buf, const size_t len){return fwrite_unlocked(buf, 1, len, file_);}
    void flush(){fflush(file_);};
    ~FileAppender(){fflush(file_);fclose(file_);};

    off_t writtenBytes() const {return writtenBytes_;}
private:
    FILE* file_;
    char buf_[64*1024];
    off_t writtenBytes_;
};


}

#endif