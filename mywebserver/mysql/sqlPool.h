#ifndef MYWEBSERVER_MYSQL_SQLCONNECTIONPOOL_H
#define MYWEBSERVER_MYSQL_SQLCONNECTIONPOOL_H

#include <mysql/mysql.h>
#include <string>

using namespace std;
namespace mywebserver{


namespace mysql{


class SqlServer;
MYSQL* initMysqlConnection();
void establishMysqlConnection(MYSQL* conn, SqlServer& server);
void testMysqlConnection(SqlServer& server);
void closeMysqlConnection(MYSQL* conn);


// inline MYSQL* createMysqlConnection(SqlServer& server){
//     MYSQL* conn = mysql::initMysqlConnection();
//     mysql::establishMysqlConnection(conn, server);
//     return conn;
// }

// inline void destroyMysqlConnection(MYSQL* conn){
//     mysql_close(conn);
// }



struct SqlServer{
    string name;
    string pwd;
    string db;

    string host;
    int port;

    MYSQL* createConnection(){
        MYSQL* conn = mysql::initMysqlConnection();
        mysql::establishMysqlConnection(conn, *this);
        return conn;
    }
    void destroyConnection(MYSQL* conn){
        mysql_close(conn);
    }
};

}




// class SqlConnectionPool:Noncopyable{
// public:
//     SqlConnectionPool(int connNum, SqlUser user, const char* db, unsigned int port)
//     :   mutex_(),
//         hasConn_(mutex_),
//         host_(user.host.c_str()),
//         user_(user.name.c_str()),
//         pwd_(user.pwd.c_str()),
//         db_(db),
//         port_(port),
//         connNum_(0),
//         autoCreateMaxNum_(0)
//     {
//         //首先确定服务器可连接/用户和数据库存在。
//         mysql::testMysqlConnection(host_.c_str(), user_.c_str(), pwd_.c_str(), db_.c_str(), port);

//         createNewConnections(connNum, false);
//         assert(connNum_ == connNum);
//     }

    

//     void setAutoCreateNewConnections(int maxConnectionsNum){
//         MutexLockGuard lock(mutex_);
//         autoCreateMaxNum_ = maxConnectionsNum;
//     }
//     void closeAutoCreateNewConnections(){
//         MutexLockGuard lock(mutex_);
//         autoCreateMaxNum_ = 0;
//     }
//     int getConnectionNum(){
//         return connNum_.load();
//     }


//     void createConnections(int connNum){
//         createNewConnections(connNum, true);
//     }

//     //如果开启了AutoCreateNewConnections并且设置了blockingMs，则会在等待时间超时后自动创建新连接。
//     MYSQL* getConnection(bool blocking, int blockingMs = 0){
//         if(blocking){
//             MutexLockGuard lock(mutex_);
//             while(connections_.size() == 0){
//                 LOG_DEBUG << "Waiting for mysql connection.";
//                 assert(blockingMs >= 0);
//                 if(blockingMs>0){
//                     bool timeout = hasConn_.waitMilliseconds(blockingMs);
//                     if(timeout){
//                         if(connNum_.load() < autoCreateMaxNum_){
//                             createNewConnections(1, false);
//                         }else{
//                             LOG_WARN << "SqlConnectionPool - number of connections reaches max";
//                         }
//                     }
//                 }else{
//                     hasConn_.wait();
//                 }
//             }
            
//             MYSQL* conn = connections_.back();
//             connections_.pop_back();
            
//             return conn;
//         }else{
//             MutexLockGuard lock(mutex_);
//             if(connections_.size() == 0 && connNum_.load() >= autoCreateMaxNum_){
//                 return NULL;
//             }else{
//                 if(connections_.size() == 0){
//                     createNewConnections(1, false);
//                 }
//                 MYSQL* conn = connections_.back();
//                 connections_.pop_back();

//                 return conn;
//             }
//         }
//     }
//     void putConnection(MYSQL* conn){
//         MutexLockGuard lock(mutex_);
//         if(std::find(connections_.begin(), connections_.end(), conn) == connections_.end()){
//             connections_.push_back(conn);
//             hasConn_.notify();
//         }else{
//             LOG_FATAL << "Failed in putConnection. Connection exists!";
//         }
//     }

//     ~SqlConnectionPool(){
//         for(auto conn:connections_){
//             mysql_close(conn);
//         }
//     }
// private:
//     // void testConnection(){
//     //     auto conn = detail::initMysqlConnection();
//     //     detail::establishMysqlConnection(conn, host_.c_str(), user_.c_str(), pwd_.c_str(), NULL, 3306);
//     //     detail::closeMysqlConnection(conn);

//     //     auto conn = detail::initMysqlConnection();
//     //     detail::establishMysqlConnection(conn, host_.c_str(), user_.c_str(), pwd_.c_str(), NULL, 3306);
//     //     detail::closeMysqlConnection(conn);
//     // }
//     void createNewConnections(int connNum, bool notify){
//         std::vector<MYSQL*> newConnections;
//         newConnections.reserve(connNum);
//         for(int i=0;i<connNum;i++){
//             MYSQL* conn = mysql::initMysqlConnection();
//             mysql::establishMysqlConnection(conn, host_.c_str(), user_.c_str(), pwd_.c_str(), db_.c_str(), port_);
//             newConnections.push_back(conn);
//         }

//         MutexLockGuard lock(mutex_);
//         //connNum_ += connNum;
//         LOG_DEBUG << "created " << newConnections.size() << " new connections";
//         //否，虽然知道load不会被fetch_add撕裂，但不知道是否会被普通+操作撕裂。
//         connNum_.fetch_add(connNum);
//         connections_.reserve(connNum_);
//         connections_.insert(connections_.end(),
//                             make_move_iterator(newConnections.begin()), 
//                             make_move_iterator(newConnections.end()));
        
//         if(notify){
//             if(connNum == 1){
//                 hasConn_.notify();
//             }else{
//                 hasConn_.notifyAll();
//             }
//         }
//     }

//     std::vector<MYSQL*> connections_;
//     std::atomic<int> connNum_;
//     int autoCreateMaxNum_;
//     MutexLock mutex_;
//     Condition hasConn_;

//     string host_;
//     string user_;
//     string pwd_;
//     string db_;
//     int port_;
// };


}
#endif
