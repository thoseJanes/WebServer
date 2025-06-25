#include "../mysql/sqlConnectionGuard.h"
#include "../mysql/sqlConnectionPool.h"

using namespace mynetlib;
int main(){
    Global::setGlobalLogLevel(Logger::Debug);
    
    // 关闭连接
    //mysql_close(conn);

    SqlConnectionPool sqlPool(5, "127.0.0.1", "testHttp", "123456", "test_http_database", 3306);

    {
        printf("1\n");
        SqlConnectionGuard sqlConn(sqlPool, true);
        printf("2\n");
        sqlConn.query(
            "CREATE TABLE IF NOT EXISTS users(" \
            "id INT AUTO_INCREMENT PRIMARY KEY," \
            "username VARCHAR(50) NOT NULL UNIQUE," \
            "password VARCHAR(50) NOT NULL," \
            "created_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP" \
        ");");
        printf("3\n");
        sqlConn.query("INSERT INTO users (username, password) VALUES ('ROOT','123456');");
        printf("4\n");
        sqlConn.query("SELECT username, password FROM users WHERE username = 'ROOT';");
        auto row = sqlConn.fetchRow();
        if(row == NULL){
            LOG_INFO << "line out";
        }else{
            LOG_INFO << "username:" << row[0] << ", password:" << row[1];
        }
        
    }
    


}