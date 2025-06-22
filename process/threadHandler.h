#ifndef WEBSERVER_THREAD_THREAD_H
#define WEBSERVER_THREAD_THREAD_H
#include "currentThread.h"

#include <functional>
#include <string>
#include <sys/prctl.h>
#include <atomic>

#include "../common/countDownLatch.h"

using namespace std;

namespace webserver{

struct ThreadData{
    string name;
    function<void()> func;
    function<void()> threadInitCallback;
    CountDownLatch* latch;
    pid_t* getTid;
};

void* runInThread(void* threadData);

class ThreadHandler:Noncopyable{
public:
    ThreadHandler(function<void()> func, string_view name = "")
    :func_(std::move(func)), name_(name), started_(false), joined_(false), handlerThreadTid_(CurrentThread::tid()){
        if(name.empty()){
            initName();
        }
    }
    void start(){
        assertInHandlerThread();
        assert(!started_);
        CountDownLatch latch_(1);
        ThreadData data{name_, func_, threadInitCallback_, &latch_, &tid_};
        if(0 == pthread_create(&thread_, NULL, runInThread, &data)){
            started_ = true;
            latch_.wait();//threadHandler只会被同一个线程操作吗？
        }else{
            fprintf(stderr, "pthread_create failed.");
            abort();
        }
    }
    ~ThreadHandler(){
        assertInHandlerThread();
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
    bool joined(){
        return joined_;
    }
    bool started(){
        return started_;
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
    void setName(string& name){
        assertInHandlerThread();
        assert(!started_);
        name_ = name;
    }
    string getName(){
        return name_;
    }
    void setThreadFunc(function<void()> cb){
        assertInHandlerThread();
        assert(!started_);
        func_ = cb;
    }
    void setInitThreadCallback(function<void()> cb){
        assertInHandlerThread();
        assert(!started_);
        threadInitCallback_ = cb;
    }
    void assertInHandlerThread(){
        assert(handlerThreadTid_ == CurrentThread::tid());
    }
private:
    pthread_t thread_;
    pid_t handlerThreadTid_;
    pid_t tid_;
    string name_;
    function<void()> func_;
    function<void()> threadInitCallback_;
    static atomic<int> counter;
    bool started_;
    bool joined_;
};





}


#endif