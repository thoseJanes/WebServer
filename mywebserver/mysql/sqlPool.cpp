#include "sqlPool.h"
#include <string>
#include "../../mynetbase/logging/logger.h"

using namespace mywebserver;

MYSQL* mysql::initMysqlConnection(){
    MYSQL* conn = mysql_init(NULL);
    if(conn){
        return conn;
    }
    LOG_FATAL << "Failed in initializing mysql connection";
    return NULL;
}

void mysql::establishMysqlConnection(MYSQL* conn, SqlServer& server){
    if(!mysql_real_connect(conn, server.host.c_str(), server.name.c_str(), server.pwd.c_str(), server.db.c_str(), server.port, NULL, 0)){
        mysql_close(conn);
        LOG_FATAL << "Failed in establishing mysql connection。 error:" << mysql_error(conn);
    }
}

void mysql::testMysqlConnection(SqlServer& server){
    auto conn = initMysqlConnection();
    if(!mysql_real_connect(conn, server.host.c_str(), server.name.c_str(), server.pwd.c_str(), NULL, server.port, NULL, 0)){
        LOG_FATAL << "Failed in establishing mysql connection. error:" << mysql_error(conn);
    }
    mysql_close(conn);

    conn = initMysqlConnection();
    if(!mysql_real_connect(conn, server.host.c_str(), server.name.c_str(), server.pwd.c_str(), server.db.c_str(), server.port, NULL, 0)){
        LOG_FATAL << "Failed in establishing mysql connection. error:" << mysql_error(conn);
    }
    mysql_close(conn);
}

void mysql::closeMysqlConnection(MYSQL* conn){
    mysql_close(conn);
}

