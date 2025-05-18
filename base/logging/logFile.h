#ifndef WEBSERVER_LOGGING_LOGFILE_H
#define WEBSERVER_LOGGING_LOGFILE_H
#include "../fileUtil/fileUtil.h"
#include <memory>
#include "../common/mutex.h"

namespace webserver{

using namespace std;

class LogFile{
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
    unique_ptr<Mutex> mutex_;

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