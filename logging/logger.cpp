#include "logger.h"
#include "./thread/currentThread.h"
#include "./time/timeStamp.h"
#include <map>

namespace webserver{

namespace detail{
std::map<Logger::LogLevel, string> LogLevel2String = {
    {Logger::Trace, "Trace "},
    {Logger::Debug, "Debug "},
    {Logger::Info , "Info  "},
    {Logger::Warn , "Warn  "},
    {Logger::Error, "Error "},
    {Logger::Fatal, "Fatal "},
};

void defaultLoggerOutputFunc(const char* buf, int len){
    fwrite(buf, 1, len, stdout);
};

void defaultLoggerFlushFunc(){
    fflush(stdout);
};
}

//time tidAndLoglevel [errno ]content - fileName:line
Logger::Logger(Logger::SourceFile sourceFile, int line, Logger::LogLevel level):
        outputFunc_(detail::defaultLoggerOutputFunc), flushFunc_(detail::defaultLoggerFlushFunc),
        level_(level), source_(sourceFile), line_(line){
    stream_ << TimeStamp::now().getMicroSecondsSinceEpoch() << " " 
            << string_view(CurrentThread::tidString(), CurrentThread::tidStringLen())
            << string_view(detail::LogLevel2String[level].c_str(), 6);
}

Logger::~Logger(){
    //stream_.withFormat(" - %s:%d\n", sourceFile, line);
    stream_ << " - " << source_ << ":" << line_ << "\n";//时间？

    outputFunc_(stream_.buffer().data(), stream_.buffer().writtenBytes());
    if(level_ == Fatal){
        flushFunc_();
        abort();
    }
}

}