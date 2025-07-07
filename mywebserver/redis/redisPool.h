#ifndef MYWEBSERVER_REDIS_REDISPOOL_H
#define MYWEBSERVER_REDIS_REDISPOOL_H

#include <hiredis/hiredis.h>
#include <string>
#include "../../mybase/logging/logger.h"
using namespace std;
namespace mywebserver{

namespace redis{
struct RedisServer{
    string host;
    int port;

    redisContext* createConnection(){
        auto conn = redisConnect(this->host.c_str(), this->port);
        if(!conn){
            LOG_FATAL << "createRedisConnection() - failed in creating redis connection.";
        }
        return conn;
    }

    void destroyConnection(redisContext* conn){
        redisFree(conn);
    }
};

// inline redisContext* createRedisConnection(RedisServer& serverInfo){
//     auto conn = redisConnect(serverInfo.host.c_str(), serverInfo.port);
//     if(!conn){
//         LOG_FATAL << "createRedisConnection() - failed in creating redis connection.";
//     }
//     return conn;
// }

// inline void destroyRedisConnection(redisContext* conn){
//     redisFree(conn);
// }

}


}



#endif