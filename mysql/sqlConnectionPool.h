#ifndef WEBSERVER_MYSQL_SQLCONNECTIONPOOL_H
#define WEBSERVER_MYSQL_SQLCONNECTIONPOOL_H

#include <mysql/mysql.h>
#include <mysql_connection.h>
#include <set>
#include <queue>
#include <atomic>
#include <algorithm>
#include <memory>
#include "../logging/logger.h"
#include "../common/condition.h"

using namespace std;
namespace webserver{


namespace detail{

MYSQL* initMysqlConnection(){
    MYSQL* conn = mysql_init(NULL);
    if(conn){
        return conn;
    }
    LOG_FATAL << "Failed in initializing mysql connection";
    return NULL;
}

void establishMysqlConnection(MYSQL* conn, const char* host, const char* user, const char* pwd, const char* db, unsigned int port){
    if(!mysql_real_connect(conn, host, user, pwd, db, port, NULL, 0)){
        mysql_close(conn);
        LOG_FATAL << "Failed in establishing mysql connection。 error:" << mysql_error(conn);
    }
}

void testMysqlConnection(const char* host, const char* user, const char* pwd, const char* db, unsigned int port){
    auto conn = initMysqlConnection();
    if(!mysql_real_connect(conn, host, user, pwd, NULL, port, NULL, 0)){
        LOG_FATAL << "Failed in establishing mysql connection. error:" << mysql_error(conn);
    }
    mysql_close(conn);

    conn = initMysqlConnection();
    if(!mysql_real_connect(conn, host, user, pwd, NULL, port, NULL, 0)){
        LOG_FATAL << "Failed in establishing mysql connection. error:" << mysql_error(conn);
    }
    mysql_close(conn);
}

void closeMysqlConnection(MYSQL* conn){
    mysql_close(conn);
}


}

class SqlConnection;
class SqlConnectionPool{
public:
    SqlConnectionPool(int connNum, const char* host, const char* user, const char* pwd, const char* db, unsigned int port)
    :   mutex_(),
        hasConn_(mutex_),
        host_(host),
        user_(user),
        pwd_(pwd),
        db_(db),
        port_(port),
        connNum_(0),
        autoCreateMaxNum_(0)
    {
        //首先确定服务器可连接/用户和数据库存在。
        detail::testMysqlConnection(host_.c_str(), user_.c_str(), pwd_.c_str(), db_.c_str(), port);

        createNewConnections(connNum, false);
        assert(connNum_ == connNum);
    }

    

    void setAutoCreateNewConnections(int maxConnectionsNum){
        MutexLockGuard lock(mutex_);
        autoCreateMaxNum_ = maxConnectionsNum;
    }
    void closeAutoCreateNewConnections(){
        MutexLockGuard lock(mutex_);
        autoCreateMaxNum_ = 0;
    }
    int getConnectionNum(){
        return connNum_.load();
    }


    void createConnections(int connNum){
        createNewConnections(connNum, true);
    }

    MYSQL* getConnection(bool blocking){
        if(blocking){
            MutexLockGuard lock(mutex_);
            while(connections_.size() == 0){
                if(connNum_.load() < autoCreateMaxNum_){
                    createNewConnections(1, false);
                }else{
                    LOG_DEBUG << "Waiting for mysql connection.";
                    hasConn_.wait();
                }
            }
            
            MYSQL* conn = connections_.back();
            connections_.pop_back();
            
            return conn;
        }else{
            MutexLockGuard lock(mutex_);
            if(connections_.size() == 0 && connNum_.load() >= autoCreateMaxNum_){
                return NULL;
            }else{
                if(connections_.size() == 0){
                    createNewConnections(1, false);
                }
                MYSQL* conn = connections_.back();
                connections_.pop_back();

                return conn;
            }
        }
    }
    void putConnection(MYSQL* conn){
        MutexLockGuard lock(mutex_);
        if(std::find(connections_.begin(), connections_.end(), conn) == connections_.end()){
            connections_.push_back(conn);
            hasConn_.notify();
        }else{
            LOG_FATAL << "Failed in putConnection. Connection exists!";
        }
    }

    ~SqlConnectionPool(){
        for(auto conn:connections_){
            mysql_close(conn);
        }
    }
private:
    // void testConnection(){
    //     auto conn = detail::initMysqlConnection();
    //     detail::establishMysqlConnection(conn, host_.c_str(), user_.c_str(), pwd_.c_str(), NULL, 3306);
    //     detail::closeMysqlConnection(conn);

    //     auto conn = detail::initMysqlConnection();
    //     detail::establishMysqlConnection(conn, host_.c_str(), user_.c_str(), pwd_.c_str(), NULL, 3306);
    //     detail::closeMysqlConnection(conn);
    // }
    void createNewConnections(int connNum, bool notify){
        std::vector<MYSQL*> newConnections;
        newConnections.reserve(connNum);
        for(int i=0;i<connNum;i++){
            MYSQL* conn = detail::initMysqlConnection();
            detail::establishMysqlConnection(conn, host_.c_str(), user_.c_str(), pwd_.c_str(), db_.c_str(), port_);
            newConnections.push_back(conn);
        }

        MutexLockGuard lock(mutex_);
        //connNum_ += connNum;
        //LOG_DEBUG << "created " << newConnections.size() << " connections";
        //否，虽然知道load不会被fetch_add撕裂，但不知道是否会被普通+操作撕裂。
        connNum_.fetch_add(connNum);
        connections_.reserve(connNum_);
        connections_.insert(connections_.end(),
                            make_move_iterator(newConnections.begin()), 
                            make_move_iterator(newConnections.end()));
        
        if(notify){
            if(connNum == 1){
                hasConn_.notify();
            }else{
                hasConn_.notifyAll();
            }
        }
    }

    std::vector<MYSQL*> connections_;
    std::atomic<int> connNum_;
    int autoCreateMaxNum_;
    MutexLock mutex_;
    Condition hasConn_;

    string host_;
    string user_;
    string pwd_;
    string db_;
    int port_;
};


}
#endif