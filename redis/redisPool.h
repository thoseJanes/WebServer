#ifndef WEBSERVER_REDIS_REDISPOOL_H
#define WEBSERVER_REDIS_REDISPOOL_H

#include <hiredis/hiredis.h>
#include <string>
using namespace std;
namespace webserver{

namespace redis{
struct RedisServer{
string host;
int port;
};

inline redisContext* createRedisConnection(RedisServer& serverInfo){
    auto conn = redisConnect(serverInfo.host.c_str(), serverInfo.port);
    return conn;
}

inline void destroyRedisConnection(redisContext* conn){
    redisFree(conn);
}

}


}



#endif