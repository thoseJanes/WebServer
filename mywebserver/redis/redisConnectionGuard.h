#ifndef MYWEBSERVER_REDIS_REDISCONNECTIONGUARD_H
#define MYWEBSERVER_REDIS_REDISCONNECTIONGUARD_H

#include <cstdarg>
#include <hiredis/hiredis.h>
#include "redisPool.h"
#include "../../mybase/common/strStream.h"
#include "../../mybase/common/connectionPool.h"
#include <stdarg.h>//va_list 变参数


using namespace mynetlib;
namespace mywebserver{

typedef ConnectionPool<redisContext, redis::RedisServer> RedisConnectionPool;

class RedisConnectionGuard:Noncopyable{//这个类可以在SqlConnectionPool内部创建?或者声明为SqlConnectionPool的友元。
//应当单线程使用。要传入loop吗？但是loop无法获取结果。
//所以应当在栈上使用。
//问题是，这样只能用于连接池的连接，如果用单个连接则无法使用。如果只使用单个连接，则没必要频繁地创建/销毁/套马甲。算了，先这样用，至少不用手动释放result。
public:
    //从连接池获取连接
    RedisConnectionGuard(RedisConnectionPool* redisPool, bool blockingGet = true, int blockingMs = 0):redisPool_(redisPool), result_(NULL), queuedNum_(0){
        connection_ = redisPool_->getConnection(blockingGet, blockingMs);
    }
    //直接传入连接。
    RedisConnectionGuard(redisContext* connection):redisPool_(nullptr), result_(NULL), queuedNum_(0){
        connection_ = connection;
    }
    ~RedisConnectionGuard(){
        if(valid()){
            if(result_){
                freeReplyObject(result_);
            }
            if(redisPool_){
                redisPool_->putConnection(connection_);
            }
        }
    }
    bool valid(){
        return connection_!=NULL;
    }

    redisReply* freeReplyAndCommand(const char* fmt, ...){
        if(result_){
            freeReplyObject(result_);
        }
        va_list args;
        va_start(args, fmt);
        result_ = static_cast<redisReply*>(redisvCommand(connection_, fmt, args));
        va_end(args);
        return result_;
    }
    void appendCommand(const char* fmt, ...){
        va_list args;
        va_start(args, fmt);
        redisvAppendCommand(connection_, fmt, args);
        va_end(args);
        queuedNum_++;
    }
    void discardReply(int times = 1){
        for(int i=0;i<times;i++){
            redisGetReply(connection_, NULL);
        }
    }
    redisReply* getTheNthReply(int n = 1){
        discardReply(n-1);
        redisReply* reply;
        redisGetReply(connection_, (void**)&reply);
        return reply;
    }
    redisReply* getTheLastReply(){
        if(__builtin_expect(queuedNum_ > 0, true)){
            discardReply(queuedNum_-1);
            redisReply* reply;
            redisGetReply(connection_, (void**)&reply);
            queuedNum_ = 0;
            return reply;
        }else{
            LOG_WARN << "RedisConnectionGuard::getTheLastReply - has no queued reply";
            return NULL;
        }
    }



private:
    redisContext* connection_;
    redisReply* result_;
    RedisConnectionPool* redisPool_;
    int queuedNum_;
};

}

#define RedisConnectionGuard(pool, blocking) error "RedisConnectionGuard should be created on the stack."

#endif