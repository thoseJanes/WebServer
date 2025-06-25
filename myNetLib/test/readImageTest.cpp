#include <stdio.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <string>
#include <string_view>

#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <sys/stat.h>

using namespace std;
class SmallFileReader{
public:
    SmallFileReader(string path, int maxSize = 64*1024-1):fd_(open(path.c_str(), O_CLOEXEC|O_RDONLY)){
        buf_[0] = '\0';
        assert(fd_ >= 0);
        
        toBuffer(maxSize);
    }
    string_view toStringView();
private:
    int toBuffer(int maxSize);
    string toString(){
        return string(buf_);
    }
    int fd_;
    char buf_[64*1024];
};

int SmallFileReader::toBuffer(int maxSize){
    if(fd_< 0){
        return errno;
    }

    auto err = 0;
    struct stat statBuf;
    int len = min<int>(sizeof(buf_)-1, maxSize);
    if(fstat(fd_, &statBuf)){
        err = errno;
    }else{
        if(S_ISREG(statBuf.st_mode)){
            //len = min<int>(len, statBuf.st_size);//读取proc/self/stat时，大小为0？？
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
    printf("%ld\n", readLen);
    return err;
}

string_view SmallFileReader::toStringView(){
    return string_view(buf_, strlen(buf_));
}

int main(){
    SmallFileReader readFile("/home/eleanor-taylor/work/C++/webServer/test/testResource/favicon32.png", 64*64*1024);

}
