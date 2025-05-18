#include "logFile.h"
#include <time.h>
#include "./process/currentProcess.h"
namespace webserver{

LogFile::LogFile(string& baseName, off_t maxSize, int maxLines, int flushInterval, bool threadSafe):
        maxSize_(maxSize), maxLines_(maxLines), flushInterval_(flushInterval),
        lastFlush_(0), lastRoll_(0), writtenLines_(0), baseName_(baseName), 
        mutex_(threadSafe? new Mutex() : NULL), now_(time(NULL))
{
    assert(baseName_.find('/') == string::npos);
    now_ = time(NULL);
    rollFile(makeFileName());//为什么这里不需要加锁？
}

void LogFile::append(const char* logline, int len){
    if(mutex_){
        MutexGuard lock(*mutex_);
        append_unlock(logline, len);
    }else{
        append_unlock(logline, len);
    }
}

void LogFile::flush(){
    if(mutex_){
        MutexGuard lock(*mutex_);
        flush_unlock();
    }else{
        flush_unlock();
    }
}

void LogFile::append_unlock(const char* logline, int len){
    file_->append(logline, len);
    writtenLines_ ++;
    now_ = time(NULL);

    if(judgeRoll()){
        flush_unlock();
        rollFile(makeFileName());
    }else if(judgeFlush()){
        flush_unlock();
    }
}

inline void LogFile::flush_unlock(){
    file_->flush();
    lastFlush_ = now_;
}

void LogFile::rollFile(string fileName){
    file_.reset(new FileAppender(baseName_));
    lastRoll_ = now_;
    writtenLines_ = 0;
}

//baseName.time.hostname.pid.log
string LogFile::makeFileName(){
    tm tmStruct;
    char timeString[32];
    gmtime_r(&now_, &tmStruct);

    strftime(timeString, sizeof timeString, ".%Y%m%d-%H%M%S.", &tmStruct);
    
    return baseName_ + timeString + currentProcess::pidString() + ".log";
}

inline bool LogFile::judgeRoll(){
    if(file_->writtenBytes()>maxSize_ || writtenLines_>maxLines_){
        return true;
    }
    return false;
}

inline bool LogFile::judgeFlush(){
    if(now_-lastFlush_ > flushInterval_){
        return true;
    }
    return false;
}
}