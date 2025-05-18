#ifndef WEBSERVER_LOGGING_LOGFILE_H
#define WEBSERVER_LOGGING_LOGFILE_H
#include "./fileUtil/fileUtil.h"
#include <memory>
#include <./common/mutex.h>

namespace webserver{

using namespace std;

class LogFile{
    LogFile(string& baseName, off_t maxSize, int maxLines, int flushInterval, bool threadSafe);
    void append(const char* logline, int len);
    void flush();
private:
    void append_unlock(const char* logline, int len);
    void flush_unlock();
    bool judgeRoll();
    bool judgeFlush();

    void rollFile(string fileName);
    string makeFileName();

    string baseName_;
    unique_ptr<FileAppender> file_;
    unique_ptr<Mutex> mutex_;

    int lastFlush_;
    int lastRoll_;
    int writtenLines_;
    
    int flushInterval_;
    //static const int rollPeriod_ = 60*60*24;
    off_t maxSize_;
    int maxLines_;

    time_t now_;
};


}

#endif