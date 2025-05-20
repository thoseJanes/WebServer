#ifndef WEBSERVER_THREAD_THREAD_H
#define WEBSERVER_THREAD_THREAD_H
#include <functional>
#include <string>
#include <sys/prctl.h>
#include "currentThread.h"
#include "../common/countDownLatch.h"

using namespace std;

namespace webserver{

struct ThreadData{
    function<void()> func;
    string name;
    CountDownLatch* latch;
    pid_t* getTid;
};

class ThreadHandler{
public:
    ThreadHandler(){}
    void start(function<void()> func){
        CountDownLatch latch_(1);
        ThreadData data{func, name_, &latch_, &tid_};
        if(0 == pthread_create(&thread_, NULL, runInThread, &data)){
            latch_.wait();
        }else{
            fprintf(stderr, "pthread_create failed.");
            abort();
        }
    }
    // void join(){
    //     pthread_join(thread_, NULL);
    // }
    void detach(){
        if(pthread_detach(thread_)){
            fprintf(stderr, "fail to detach thread.");
            abort();
        }
    }
    void initName(){
        //用原子操作。也可用锁。
    }
    void setName(string name){
        name_ = std::move(name);
    }
private:
    pthread_t thread_;
    pid_t tid_;
    string name_;
};



void* runInThread(void* threadData){
    ThreadData* data = reinterpret_cast<ThreadData*>(threadData);

    *(data->getTid) = CurrentThread::tid();
    prctl(PR_SET_NAME, data->name.c_str());
    auto func = data->func;
    data->latch->countDown();//由于data存储在栈上，countDown()之后，data会被删除。

    try{
        func();
    }
    catch(std::exception e){
        fprintf(stderr, "%s", e.what());
        abort();
    }
    catch(...){
        fprintf(stderr, "unknow error.");
        abort();
    }

    return NULL;
}

}


#endif