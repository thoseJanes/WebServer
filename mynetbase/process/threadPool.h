#ifndef MYNETLIB_THREAD_THREADPOOL_H
#define MYNETLIB_THREAD_THREADPOOL_H
#include "threadHandler.h"

#include <deque>
#include <functional>
#include <memory>

#include "../logging/logger.h"
namespace mynetlib{


//1、start和stop和线程池构造在同一线程进行。否。stop也不一定和构造在同一线程（析构函数是在什么线程进行的？）。严格来说并不是线程安全，但start和stop一般时间间隔很长不会相互影响？？？
//2、stop后不可再start
class ThreadPool:Noncopyable{
public:
    typedef std::function<void()> Task;
    ThreadPool(int threadNum, int maxTasks):mutex_(), notEmpty_(mutex_), notFull_(mutex_), running_(false),
            threadNum_(threadNum), maxTasks_(maxTasks){
        assert(threadNum > 0 && maxTasks > 0);//至少创建一个线程并可以运行一个任务？
    };
    void start(){
        assert(threads_.size() == 0);//防止线程池再次运行。
        running_ = true;
        for(int i=0;i<threadNum_;i++){
            threads_.emplace_back(new ThreadHandler(std::bind(&ThreadPool::threadFunc, this)));//是否可以只bind一次然后传参？
            threads_[i]->start();
        }
    }

    //即使不在run也能放入任务。但是不能超过最大任务数。
    void put(Task task){
        if(threadNum_){
            MutexLockGuard lock(mutex_);
            while(tasks_.size()>=maxTasks_ && running_){
                notFull_.wait();
            }
            if(tasks_.size()<maxTasks_){
                tasks_.push_back(std::move(task));
                notEmpty_.notify();
            }else{
                LOG_ERROR << "threadPool::run() discard a task when running is "<< running_;
            }
        }
    }

    void stop(){
        assert(running_);
        running_ = false;
        notEmpty_.notifyAll();
        notFull_.notifyAll();
        for(unique_ptr<ThreadHandler>& thread:threads_){
            thread->join();//等待所有线程结束。
        }
        //threads.clear();为了防止线程池第二次运行start，不加入clear。
    }

private:
    Task take(){
        MutexLockGuard lock(mutex_);
        while(tasks_.empty() && running_){
            notEmpty_.wait();
        }
        if(!running_){return NULL;}
        //assert(!tasks.empty());
        Task task = tasks_.front();
        tasks_.pop_front();
        notFull_.notify();
        return task;
    }
    void threadFunc(){
        while(running_){
            try{
                Task task(take());
                if(task){
                    task();
                }
            }catch(...){//一系列错误处理。
                abort();
            }
        }
    }
    
    std::deque<Task> tasks_;
    std::vector<unique_ptr<ThreadHandler>> threads_;
    MutexLock mutex_;
    Condition notEmpty_;
    Condition notFull_;
    bool running_;
    int threadNum_;
    int maxTasks_;
};



}







#endif