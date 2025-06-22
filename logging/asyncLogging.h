#ifndef WEBSERVER_LOGGING_ASYNCLOGGING_H
#define WEBSERVER_LOGGING_ASYNCLOGGING_H
#include "logFile.h"
#include "../common/strStream.h"
#include "../process/threadHandler.h"
#include <memory>
using namespace std;
namespace webserver{

class AsyncLogging:Noncopyable{
public:
AsyncLogging(string baseName, int rollSize, int flushInterval);
~AsyncLogging();
void append(const char* logline, int len);
void start();
void stop();
//void flush();不应该提供flush！因为这可能导致竞态条件

private:
void loggingThreadFunc();
LogFile output_;
ThreadHandler thread_;
typedef detail::StreamBuffer<4000*1000> Buffer;
vector<unique_ptr<Buffer>> buffers_;
unique_ptr<Buffer> currentBuffer_;
unique_ptr<Buffer> nextBuffer_;
MutexLock mutex_;
Condition notEmpty_;
bool running_;
//int rollSize_;
int flushInterval_;

};


}




#endif