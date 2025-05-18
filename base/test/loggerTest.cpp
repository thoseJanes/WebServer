
#include "../logging/logger.h"
#include "../logging/logFile.h"
#include "stdio.h"
#include <stdarg.h>//va_list 变参数
#include <memory>
#include <string.h>
#include <string>

#include <functional>

using namespace webserver;
std::string str = std::string("testLogFile");
LogFile logFile(str);


void LoggerTest(){
    LOG_INFO << "\n";
    #define TestCaseGenerator(tp, value, fmt) \
    {tp vl = value; \
    LOG_INFO << #tp " test:" << vl; \
    printf("printf value:" fmt "\n", vl);}

    TestCaseGenerator(short, 2025, "%d");
    TestCaseGenerator(int, 20250518, "%d");
    TestCaseGenerator(long, 20250518, "%ld");
    TestCaseGenerator(long long, 20250518, "%lld");
    #undef TestCaseGenerator

    {
        char test = 'c';
        LOG_INFO << "char test:" << test;
        printf("printf value:%c\n", test);
    }

    {
        char test[] = "-test-c-string-";
        LOG_INFO << "c string test:" << test;
        printf("printf value:%s\n", test);
    }

    {
        string test = "-test-string-";
        LOG_INFO << "string test:" << test;
        printf("printf value:%s\n", test.c_str());
    }

    

    {float fl = 3.1415926535397932;
    LOG_INFO << "float test:" << fl;
    printf("printf value:%.*g\n", webserver::LogStream::getPrecision(), fl);}

    {double dl = 3.1415926535397932;
    LOG_INFO << "double test:" << dl;
    printf("printf value:%.*g\n", webserver::LogStream::getPrecision(), dl);}

    {long double ld = 3.1415926535397932;
    LOG_INFO << "double test:" << ld;
    printf("printf value:%.*Lg\n", webserver::LogStream::getPrecision(), ld);}

}

void LogFileTest(){
    //std::function<void(const char*, int)> outputFunc = [&logFile](const char* logline, int len){logFile.append(logline, len);};
    //std::function<void()> flushFunc = [&logFile](){logFile.flush();};
    auto outputFunc = [](const char* logline, int len){logFile.append(logline, len);};
    auto flushFunc = [](){logFile.flush();};
    Global::setGlobalFlushFunc(flushFunc);
    Global::setGlobalOutputFunc(outputFunc);
    LOG_INFO << "\n";
    LOG_INFO << "log file test";

}

void LogLevelTest(){
    //std::function<void(const char*, int)> outputFunc = [&logFile](const char* logline, int len){logFile.append(logline, len);};
    //std::function<void()> flushFunc = [&logFile](){logFile.flush();};
    Logger::LogLevel original = Global::getGlobalLogLevel();
    Global::setGlobalLogLevel(Logger::Debug);
    LOG_INFO << "\n";
    LOG_INFO << "log info";
    LOG_TRACE << "log trace";
    LOG_DEBUG << "log debug";


    Global::setGlobalLogLevel(original);

}

int main(){
    // printf("test log\n");
    // auto test = new Test();
    // printf("test c\n");
    // test->withFormat("%d", 300);
    LoggerTest();
    LogLevelTest();
    LogFileTest();
    

}