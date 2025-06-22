#ifndef WEBSERVER_FILEUTIL_FILEUTIL_H
#define WEBSERVER_FILEUTIL_FILEUTIL_H


#include <stdio.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "../common/stringArg.h"
#include "../common/patterns.h"

namespace webserver{

namespace detail{
class FileSource{
public:
    template<int N>
    FileSource(const char(&arr)[N]):len(N-1), name(arr){//注意字符数组长度包含最后的'\0'
        auto slash = strrchr(arr, '/');
        if(slash){
            name = slash + 1;
            len -= slash-arr;
        }
    }
    explicit FileSource(const char* arr):name(arr){//注意字符数组长度包含最后的'\0'
        auto slash = strrchr(arr, '/');
        if(slash){
            name = slash + 1;
        }
        len = static_cast<int>(strlen(name));
    }
    int len;
    const char* name;
};

class FilePath{
public:
    template<int N>
    FilePath(const char(&arr)[N]):len(N-1), dir(arr){//注意字符数组长度包含最后的'\0'
        auto slash = strrchr(arr, '/');
        if(slash){
            len = slash - arr;
        }
    }
    explicit FilePath(const char* arr):dir(arr){//注意字符数组长度包含最后的'\0'
        auto slash = strrchr(arr, '/');
        if(slash){
            len = slash - arr;
        }else{
            len = static_cast<int>(strlen(dir));
        }
    }
    string toString(){
        return string(dir, len);
    }
    string_view toStringView(){
        return string_view(dir, len);
    }
private:
    int len;
    const char* dir;
};


bool fileExists(string_view path);

int readToBuffer(int fd, char* buf, size_t maxSize, int* err);
}



class SmallFileReader:Noncopyable{
public:
    SmallFileReader(StringArg path, size_t maxSize = 64*1024-1):fd_(open(path.c_str(), O_CLOEXEC|O_RDONLY)), mayHaveMore_(true){
        buf_[0] = '\0';
        len_ = 0;
        assert(fd_ >= 0);
        
        toBuffer(maxSize);
    }
    ~SmallFileReader(){
        if(fd_ >= 0){
            ::close(fd_);
            fd_ = -1;
        }
    }
    bool mayHaveMore(){return mayHaveMore_;}
    int continueReadOverwritesBuffer();
    string_view toStringView(){
        return string_view(buf_, len_);
    }
    string toString(){
        return string(buf_);
    }
private:
    int toBuffer(size_t maxSize);
    
    int fd_;
    char buf_[64*1024];
    size_t len_;
    bool mayHaveMore_;
};

class AdaptiveFileReader:Noncopyable{
public:
    AdaptiveFileReader(StringArg path, size_t minBufferSize = 64*1024-1)
    :   fd_(open(path.c_str(), O_CLOEXEC|O_RDONLY)), 
        bufSize_(0),
        len_(0),
        mayHaveMore_(true),
        bufInit_(false)
        
    {
        assert(fd_ >= 0);
        toBuffer(minBufferSize);
    }
    ~AdaptiveFileReader(){
        if(fd_ >= 0){
            ::close(fd_);
            fd_ = -1;
        }
        if(bufInit_){
            free(buf_);
        }
    }
    bool mayHaveMore(){return mayHaveMore_;}
    int continueReadOverwritesBuffer();
    string_view toStringView(){
        return string_view(buf_, len_);
    }
    string toString(){
        return string(buf_);
    }
private:
    int toBuffer(size_t minBufferSize);
    
    int fd_;
    char* buf_;
    size_t bufSize_;
    size_t len_;
    bool mayHaveMore_;
    bool bufInit_;
};



class FileAppender:Noncopyable{
public:
    FileAppender(StringArg path):file_(fopen(path.c_str(), "ae")){
        assert(file_);
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