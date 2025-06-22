#ifndef WEBSERVER_COMMON_CONNECTIONPOOL_H
#define WEBSERVER_COMMON_CONNECTIONPOOL_H


#include <atomic>
#include <algorithm>
#include <functional>
#include "../logging/logger.h"
#include "../common/condition.h"


using namespace std;
namespace webserver{


template<typename T, typename INFO>

//有没有一种可能，连接池根本不需要增加连接数。因为只要给每个线程都创建一个连接，就不会有连接不够用的情况？
//除非要同时开启多个连接异步查询？或者线程数太多了？
class ConnectionPool:Noncopyable{
public:
    typedef std::function<void()> InitCallback;
    typedef std::function<T*(INFO&)> CreateConnectionCallback;
    typedef std::function<void(T*)> CloseConnectionCallback;
    ConnectionPool(int connNum, INFO serverInfo, 
                    CreateConnectionCallback createCallback, 
                    CloseConnectionCallback closeCallback, 
                    InitCallback initCallback=NULL)
    :   mutex_(),
        hasConn_(mutex_),
        connNum_(0),
        autoCreateMaxNum_(0),
        createCallback_(createCallback),
        closeCallback_(closeCallback),
        serverInfo_(serverInfo)
    {
        if(initCallback){
            initCallback();
        }
        
        //首先确定服务器可连接/用户和数据库存在。
        //mysql::testMysqlConnection(host_.c_str(), user_.c_str(), pwd_.c_str(), db_.c_str(), port);
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

    //如果开启了AutoCreateNewConnections并且设置了blockingMs，则会在等待时间超时后自动创建新连接。
    T* getConnection(bool blocking, int blockingMs = 0){
        if(blocking){
            MutexLockGuard lock(mutex_);
            while(connections_.size() == 0){
                LOG_DEBUG << "Waiting for mysql connection.";
                assert(blockingMs >= 0);
                if(blockingMs>0){
                    bool timeout = hasConn_.waitMilliseconds(blockingMs);
                    if(timeout){
                        if(connNum_.load() < autoCreateMaxNum_){
                            createNewConnections(1, false);
                        }else{
                            LOG_WARN << "SqlConnectionPool - number of connections reaches max";
                        }
                    }
                }else{
                    hasConn_.wait();
                }
            }
            
            T* conn = connections_.back();
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
                T* conn = connections_.back();
                connections_.pop_back();

                return conn;
            }
        }
    }
    void putConnection(T* conn){
        MutexLockGuard lock(mutex_);
        if(std::find(connections_.begin(), connections_.end(), conn) == connections_.end()){
            connections_.push_back(conn);
            hasConn_.notify();
        }else{
            LOG_FATAL << "Failed in putConnection. Connection exists!";
        }
    }

    ~ConnectionPool(){
        for(auto conn:connections_){
            closeCallback_(conn);
        }
    }
private:
    void createNewConnections(int connNum, bool notify){
        std::vector<T*> newConnections;
        newConnections.reserve(connNum);
        for(int i=0;i<connNum;i++){
            // T* conn = mysql::initMysqlConnection();
            // mysql::establishMysqlConnection(conn, host_.c_str(), user_.c_str(), pwd_.c_str(), db_.c_str(), port_);
            T* conn = createCallback_(serverInfo_);
            newConnections.push_back(conn);
        }

        MutexLockGuard lock(mutex_);
        //connNum_ += connNum;
        LOG_DEBUG << "created " << newConnections.size() << " new connections";
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

    std::vector<T*> connections_;
    std::atomic<int> connNum_;
    int autoCreateMaxNum_;
    MutexLock mutex_;
    Condition hasConn_;
    CreateConnectionCallback createCallback_;
    CloseConnectionCallback closeCallback_;

    INFO serverInfo_;
    // string host_;
    // string user_;
    // string pwd_;
    // string db_;
    // int port_;
};


}
#endif