#ifndef WEBSERVER_THREAD_THREADPOOL_H
#define WEBSERVER_THREAD_THREADPOOL_H
#include <queue>
#include <functional>
#include <threadHandler.h>
namespace webserver{

class ThreadPool{
public:
    typedef std::function<void()> Task;
    ThreadPool(int maxSize):mutex_(), isEmpty_(mutex_){

    };
    Task take(){
        MutexLockGuard lock(mutex_);
        while(tasks.empty()){
            isEmpty_.wait();
        }
        Task task = tasks.back();
        tasks.pop();
        return task;
    }

    void put(Task task){
        MutexLockGuard lock(mutex_);
        tasks.push(task);
    }

    void run(){
        while(!stop){
            if()
        }
    }

    void stop(){
        stop = true;
    }

    
    std::queue<Task> tasks;
    std::vector<ThreadHandler> threads;
    Condition isEmpty_;
    MutexLock mutex_;
    bool stop = false;
}



}







#endif