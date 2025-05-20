#ifndef WEBSERVER_LOGGING_ASYNCLOGGING_H
#define WEBSERVER_LOGGING_ASYNCLOGGING_H
#include "logFile.h"
#include "logStream.h"
#include "../thread/threadHandler.h"
#include <queue>
#include <memory>
using namespace std;
namespace webserver{

class AsyncLogging{
public:
AsyncLogging(string baseName, int flushInterval);
void append(const char* logline, int len);
void loggingThreadFunc();
void flush();

private:
LogFile output_;
ThreadHandler thread_;
typedef detail::LogBuffer<4000*1000> Buffer;
vector<unique_ptr<Buffer>> buffers_;
unique_ptr<Buffer> currentBuffer_;
unique_ptr<Buffer> nextBuffer_;
MutexLock mutex_;
Condition notEmpty_;
//int rollSize_;
int flushInterval_;

};


}




#endif