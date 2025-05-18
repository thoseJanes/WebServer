#ifndef WEBSERVER_LOGGING_LOGGER_H
#define WEBSERVER_LOGGING_LOGGER_H

#include "./patterns.h"
#include "logStream.h"


namespace webserver{

namespace detail{

}

namespace{
class SourceFile{
public:
    template<int N>
    SourceFile(const char(&arr)[N]):len(N-1), name(arr){//注意字符数组长度包含最后的'\0'
        auto slash = strrchr(arr, '/');
        if(slash){
            name = slash + 1;
            len -= slash-arr;
        }
    }
    explicit SourceFile(const char* arr):name(arr){//注意字符数组长度包含最后的'\0'
        auto slash = strrchr(arr, '/');
        if(slash){
            name = slash + 1;
        }
        len = static_cast<int>(strlen(name));
    }
    int len;
    const char* name;
};

}

inline LogStream& operator<<(LogStream& stream, const SourceFile file){
    stream.append(file.len, "%s", file.name);
    return stream;
}

class Logger:Noncopyable{
public:
    typedef void (*OutputFunc)(const char*, int);
    typedef void (*FlushFunc)();
    typedef SourceFile SourceFile;
    enum LogLevel{
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Fatal
    };
    Logger(SourceFile sourceFile, int line, LogLevel level);
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



namespace global{
    Logger::LogLevel logLevel;
    Logger::OutputFunc logOutputFunc;
    Logger::FlushFunc logFlushFunc;
}


}


#endif