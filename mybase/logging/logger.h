#ifndef MYNETLIB_LOGGING_LOGGER_H
#define MYNETLIB_LOGGING_LOGGER_H

#include "../common/patterns.h"
#include "../common/strStream.h"
#include "../common/fileUtil.h"


namespace mynetlib{


typedef StrStream<4000> LogStream;


inline LogStream& operator<<(LogStream& stream, const detail::FileSource file){
    stream.withFormat(file.len, "%s", file.name);
    return stream;
}

class Logger:Noncopyable{
public:
    typedef void (*OutputFunc)(const char*, int);
    typedef void (*FlushFunc)();
    typedef detail::FileSource SourceFile;
    enum LogLevel{
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Fatal
    };
    Logger(SourceFile sourceFile, int line, LogLevel level);
    Logger(SourceFile sourceFile, int line, bool toAbort);
    Logger(SourceFile sourceFile, int line, LogLevel level, OutputFunc outputFunc, FlushFunc flushFunc);
    
    ~Logger();
    LogStream& stream(){
        return stream_;
    }
private:
    LogStream stream_;
    LogLevel level_;
    
    SourceFile source_;
    int line_;

    OutputFunc outputFunc_;
    FlushFunc flushFunc_;
};



namespace Global{
    extern Logger::LogLevel logLevel;
    extern Logger::OutputFunc logOutputFunc;
    extern Logger::FlushFunc logFlushFunc;

    inline void setGlobalLogLevel(Logger::LogLevel level){
        logLevel = level;
    }
    inline Logger::LogLevel getGlobalLogLevel(){
        return logLevel;
    }
    inline void setGlobalOutputFunc(Logger::OutputFunc func){
        logOutputFunc = func;
    }
    inline void setGlobalFlushFunc(Logger::FlushFunc func){
        logFlushFunc = func;
    }
}


}

#define LOG_INFO if(mynetlib::Global::logLevel <= mynetlib::Logger::Info) \
    mynetlib::Logger(__FILE__, __LINE__, mynetlib::Logger::Info).stream()
#define LOG_DEBUG if(mynetlib::Global::logLevel <= mynetlib::Logger::Debug) \
    mynetlib::Logger(__FILE__, __LINE__, mynetlib::Logger::Debug).stream()
#define LOG_TRACE if(mynetlib::Global::logLevel <= mynetlib::Logger::Trace) \
    mynetlib::Logger(__FILE__, __LINE__, mynetlib::Logger::Trace).stream()

#define LOG_WARN \
    mynetlib::Logger(__FILE__, __LINE__, mynetlib::Logger::Warn).stream()
#define LOG_ERROR \
    mynetlib::Logger(__FILE__, __LINE__, mynetlib::Logger::Error).stream()
#define LOG_FATAL \
    mynetlib::Logger(__FILE__, __LINE__, mynetlib::Logger::Fatal).stream()

#define LOG_SYSERROR \
    mynetlib::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL \
    mynetlib::Logger(__FILE__, __LINE__, true).stream()


#endif