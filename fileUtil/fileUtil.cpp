#include "fileUtil.h"

#include "unistd.h"
#include "./thread/currentThread.h"
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>



namespace webserver{

// int SmallFileReader::toBuffer(){
//     assert(fd_ > 0);
//     struct stat statBuf;
//     if(fstat(fd_, &statBuf)){
//         return errno;
//     }

//     if(S_ISREG(statBuf.st_mode)){
        
//     }else if(S_ISDIR(statBuf.st_mode)){
//         return EISDIR;
//     }
    

//     auto len = statBuf.st_size;
//     auto ret = read(fd_, buf_, sizeof(buf_));
//     auto readLen = ret;
//     while(readLen < len){
//         if(ret < 0){
//             fprintf(stderr, "SmallFileReader toBuffer() read failed");
//             break;
//         }else if(ret == 0){
//             break;
//         }
//         readLen += ret;
//     }
// }

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