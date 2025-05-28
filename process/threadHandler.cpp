#include "threadHandler.h"


using namespace webserver;

atomic<int> ThreadHandler::counter(0);



void* webserver::runInThread(void* threadData){
    ThreadData* data = reinterpret_cast<ThreadData*>(threadData);

    *(data->getTid) = CurrentThread::tid();
    prctl(PR_SET_NAME, data->name.c_str());
    auto func = data->func;
    auto threadInitCallback = data->threadInitCallback;
    data->latch->countDown();//由于data存储在栈上，countDown()之后，data会被删除。

    try{
        if(threadInitCallback){
            threadInitCallback();
        }
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
