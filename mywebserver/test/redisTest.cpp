#include <hiredis/hiredis.h>
#include <hiredis/read.h>


int main(){

    redisContext* conn = redisConnect("127.0.0.1", 6379);

    redisReply* reply = static_cast<redisReply*>(redisCommand(conn, "set test::2 1"));
    reply = static_cast<redisReply*>(redisCommand(conn, "get test::2"));
    reply = static_cast<redisReply*>(redisCommand(conn, "incr test::2"));
    if(reply->type == REDIS_REPLY_STRING){
        printf("redis reply:%s\n", reply->str);
        printf("type:%d\n", reply->type);
        printf("element:%ld\n", reply->elements);
        printf("integer:%lld\n", reply->integer);
    }
    else if(reply->type == REDIS_REPLY_INTEGER){
        printf("redis reply:%lld\n", reply->integer);
    }
    else if(reply->type == REDIS_REPLY_STRING){
        printf("redis reply:%s\n", reply->str);
    }

    reply = static_cast<redisReply*>(redisCommand(conn, "del test::2"));




}