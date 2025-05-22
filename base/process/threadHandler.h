#ifndef WEBSERVER_THREAD_THREAD_H
#define WEBSERVER_THREAD_THREAD_H
#include <functional>
#include <string>
#include <sys/prctl.h>
#include <atomic>
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

class ThreadHandler{
public:
    ThreadHandler(function<void()> func, string name = NULL)
    :func_(std::move(func)), name_(name), started_(false), joined_(false){
        if(!name){
            initName();
        }
    }
    void start(){
        assert(!started_);
        CountDownLatch latch_(1);
        ThreadData data{func_, name_, &latch_, &tid_};
        if(0 == pthread_create(&thread_, NULL, runInThread, &data)){
            started_ = true;
            latch_.wait();//threadHandler只会被同一个线程操作吗？
        }else{
            fprintf(stderr, "pthread_create failed.");
            abort();
        }
    }
    ~ThreadHandler(){
        if(started_&&(!joined_)){//join后，thread_会变成NULL？不会！！
            pthread_detach(thread_);
        }
    }
    void join(){
        if(started_&&(!joined_)){
            pthread_join(thread_, NULL);
            joined_ = true;
        }
    }
    // void detach(){
    //     if(pthread_detach(thread_)){
    //         fprintf(stderr, "fail to detach thread.");
    //         abort();
    //     }
    // }
    void initName(){
        //用原子操作递增计数。也可用锁。
        counter.fetch_add(1);
        char buf[32];
        snprintf(buf, sizeof buf, "Thread%d", counter.load());
        name_ = buf;
    }
    void setName(string name){
        name_ = std::move(name);
    }
    void setFunc();
private:
    pthread_t thread_;
    pid_t tid_;
    string name_;
    function<void()> func_;
    static atomic<int> counter;
    bool started_;
    bool joined_;
};





}


#endif