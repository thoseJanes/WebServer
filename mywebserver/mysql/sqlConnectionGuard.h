#ifndef MYWEBSERVER_MYSQL_SQLCONNECTIONGUARD_H
#define MYWEBSERVER_MYSQL_SQLCONNECTIONGUARD_H

#include "sqlPool.h"
#include <mysql/mysql.h>
#include "../../mynetbase/common/strStream.h"
#include "../../mynetbase/common/connectionPool.h"


using namespace mynetlib;
namespace mywebserver{


typedef ConnectionPool<MYSQL, mysql::SqlServer> SqlConnectionPool;

typedef StrStream<512> QueryStream;
class SqlConnectionGuard:Noncopyable{//这个类可以在SqlConnectionPool内部创建?或者声明为SqlConnectionPool的友元。
//应当单线程使用。要传入loop吗？但是loop无法获取结果。
//所以应当在栈上使用。
public:
    //从连接池获取连接
    SqlConnectionGuard(SqlConnectionPool* sqlPool, bool blockingGet = true, int blockingMs = 0):sqlPool_(sqlPool), result_(NULL){
        connection_ = sqlPool_->getConnection(blockingGet, blockingMs);
    }
    //直接传入连接
    SqlConnectionGuard(MYSQL* connection, MutexLock* mutex = nullptr):sqlPool_(nullptr), result_(NULL), mutex_(mutex){
        connection_ = connection;
        if(mutex_){
            mutex_->lock();
        }
    }
    ~SqlConnectionGuard(){
        if(valid()){
            if(result_){
                mysql_free_result(result_);
            }
            if(sqlPool_){
                sqlPool_->putConnection(connection_);
            }
            if(mutex_){
                mutex_->unlock();
            }
        }
    }
    bool valid(){
        return connection_!=NULL;
    }

    QueryStream& queryStream(){
        stream_.reset();
        return stream_;
    }

    int query(){
        stream_ << '\0';
        if(stream_.isTruncated()){
            LOG_FATAL << "SqlConnectionGuard::query - Query failed with stream buffer truncated. Make sure length of query sentence is shorter than 500 bytes.";
            return -1;
            //这不是mysql本身的错误。应该怎么办呢？
        }else{
            int ret = mysql_query(connection_, getStreamBufferStart());//执行新查询时，会隐式释放一个结果集。
            if(ret){
                LOG_ERROR << "Failed in mysql query. error: " << mysql_error(connection_);
            }
            result_ = NULL;
            return ret;//由上层来处理。
        }
    }

    void asssertQuery(){
        int ret = query();//执行新查询时，会隐式释放一个结果集。
        if(ret){
            abort();
        }
    }

    int query(const char* sentence){
        int ret = mysql_query(connection_, sentence);//执行新查询时，会隐式释放一个结果集。
        if(ret){
            LOG_ERROR << "Failed in mysql query. error: " << mysql_error(connection_);
        }
        result_ = NULL;
        return ret;//由上层来处理。
        // if(ret){
        //     LOG_ERROR << "Failed in mysql query. error: " << mysql_error(connection_);
        //     return false;
        // }else{
        //     return true;
        // }
    }

    void asssertQuery(const char* sentence){
        int ret = mysql_query(connection_, sentence);//执行新查询时，会隐式释放一个结果集。
        if(ret){
            LOG_FATAL << "Failed in mysql query. error: " << mysql_error(connection_);
        }
        result_ = NULL;
    }

    size_t getNumberOfRows(){
        if(!result_){
            result_ = mysql_store_result(connection_);
            if(!result_){
                LOG_ERROR << "Failed in mysql_store_result. error: " << mysql_error(connection_);
                return 0;
            }
        }
        return mysql_num_rows(result_);
    }

    MYSQL_ROW fetchRow(){
        if(!result_){
            result_ = mysql_store_result(connection_);
            if(!result_){
                LOG_ERROR << "Failed in mysql_store_result. error: " << mysql_error(connection_);
                return NULL;
            }
        }
        return mysql_fetch_row(result_);
    }

    MYSQL* getConnection(){
        return connection_;
    }
    const char* getStreamBufferStart(){
        if(stream_.isTruncated()){
            return NULL;
        }
        return stream_.buffer().data();
    }
    size_t getStreamBufferLen(){
        return stream_.buffer().writtenBytes();
    }

private:
    MYSQL* connection_ = nullptr;
    MYSQL_RES* result_ = nullptr;
    QueryStream stream_;
    SqlConnectionPool* sqlPool_ = nullptr;
    MutexLock* mutex_ = nullptr;
};

}

#define SqlConnectionGuard(pool, blocking) error "SqlConnectionGuard should be created on the stack."

#endif
