#ifndef MYNETLIB_LOGGING_LOGFILE_H
#define MYNETLIB_LOGGING_LOGFILE_H

#include <memory>
#include "../common/fileUtil.h"
#include "../common/mutexLock.h"

namespace mynetlib{

using namespace std;

class LogFile:Noncopyable{
public:
    LogFile(string& baseName, 
            off_t maxSize = 64*1024, 
            int maxLines = 1024, 
            int flushInterval = 3, 
            bool threadSafe = true);
    void append(const char* logline, int len);
    void flush();
private:
    void append_unlock(const char* logline, int len);
    void flush_unlock();
    bool judgeRoll();
    bool judgeFlush();

    void rollFile(string fileName);
    string makeFileName();

    const string baseName_;
    unique_ptr<FileAppender> file_;
    unique_ptr<MutexLock> mutex_;

    int lastFlush_;
    int lastRoll_;
    int writtenLines_;
    
    const int flushInterval_;
    //static const int rollPeriod_ = 60*60*24;
    const off_t maxSize_;
    const int maxLines_;

    time_t now_;
};


}

#endif