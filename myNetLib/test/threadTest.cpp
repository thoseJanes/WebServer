#include <pthread.h>
#include "../common/mutexLock.h"
#include "../common/condition.h"
#include "../process/threadHandler.h"

using namespace mynetlib;


MutexLock mutex_ = MutexLock();
Condition cond_(mutex_);
bool flag = false;

void worker(){
    printf("%d\n", flag);
    MutexLockGuard lock(mutex_);
    flag = true;
}

int main(){
    printf("%d\n", flag);
    ThreadHandler handler(worker, "testThread");
    handler.start();
    
    MutexLockGuard lock(mutex_);
    while(!flag){
        cond_.wait();
    }
    printf("%d\n", flag);
}