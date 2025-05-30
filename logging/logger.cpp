#include "logger.h"
#include "../process/currentThread.h"
#include "../time/timeStamp.h"
#include <map>

namespace webserver{

namespace detail{
std::map<Logger::LogLevel, string> LogLevel2String = {
    {Logger::Trace, " Trace "},
    {Logger::Debug, " Debug "},
    {Logger::Info , " Info  "},
    {Logger::Warn , " Warn  "},
    {Logger::Error, " Error "},
    {Logger::Fatal, " Fatal "},
};

void defaultLoggerOutputFunc(const char* buf, int len){
    fwrite(buf, 1, len, stdout);
}

void defaultLoggerFlushFunc(){
    fflush(stdout);
}

Logger::LogLevel initLogLevel(){
    // return Logger::Trace;
    if(getenv("WEBSERVER_LOG_TRACE")){
        return Logger::Trace;
    }else if(getenv("WEBSERVER_LOG_DEBUG")){
        return Logger::Debug;
    }else{
        return Logger::Info;
    }
}
}

Logger::FlushFunc Global::logFlushFunc = detail::defaultLoggerFlushFunc;
Logger::OutputFunc Global::logOutputFunc = detail::defaultLoggerOutputFunc;
Logger::LogLevel Global::logLevel = detail::initLogLevel();

//time tidAndLoglevel [errno ]content - fileName:line
Logger::Logger(Logger::SourceFile sourceFile, int line, Logger::LogLevel level):
        outputFunc_(Global::logOutputFunc), flushFunc_(Global::logFlushFunc),
        level_(level), source_(sourceFile), line_(line){
    stream_ << TimeStamp::now().toFormattedString(true) << " " 
            << string_view(CurrentThread::tidString(), CurrentThread::tidStringLen())
            << string_view(detail::LogLevel2String[level].c_str(), 7);
}

Logger::Logger(Logger::SourceFile sourceFile, int line, bool toAbort):
        outputFunc_(Global::logOutputFunc), flushFunc_(Global::logFlushFunc),
        level_(toAbort?Fatal:Error), source_(sourceFile), line_(line){

    stream_ << TimeStamp::now().toFormattedString(true) << " " 
            << string_view(CurrentThread::tidString(), CurrentThread::tidStringLen())
            << string_view(detail::LogLevel2String[level_].c_str(), 7)
            << strerror_tl(errno) << " (errno=" << errno << ") ";
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