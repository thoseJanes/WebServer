#ifndef WEBSERVER_MYSQL_SQLCONNECTIONGUARD_H
#define WEBSERVER_MYSQL_SQLCONNECTIONGUARD_H

#include "sqlConnectionPool.h"

namespace webserver{


class SqlConnectionGuard{//这个类可以在SqlConnectionPool内部创建?或者声明为SqlConnectionPool的友元。
//应当单线程使用。要传入loop吗？但是loop无法获取结果。
//所以应当在栈上使用。
public:
    SqlConnectionGuard(SqlConnectionPool& sqlPool, bool blockingGet = true):sqlPool_(&sqlPool){
        connection_ = sqlPool_->getConnection(blockingGet);
    }
    ~SqlConnectionGuard(){
        if(valid()){
            if(result_){
                mysql_free_result(result_);
            }
            sqlPool_->putConnection(connection_);
        }
    }
    bool valid(){
        return connection_!=NULL;
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

private:
    MYSQL* connection_;
    MYSQL_RES* result_;
    SqlConnectionPool* sqlPool_;
};

}

#define SqlConnectionGuard(pool, blocking) error "SqlConnectionGuard should be created on the stack."

#endif