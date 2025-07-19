#ifndef MYWEBSERVER_HTTP_CONTEXTMAP_H
#define MYWEBSERVER_HTTP_CONTEXTMAP_H
#include <string>
#include <assert.h>
#include <map>
#include "../../mynetbase/common/patterns.h"

using namespace std;
class ContextMap:mynetlib::Noncopyable{
public:
    void setContext(string key, void* value) {
        assert(!hasContext(key));
        context_.insert({key, value});
    }
    bool hasContext(string key) {
        return context_.find(key) != context_.end();
    }
    void* getContext(string key) {
        assert(hasContext(key));
        return context_.at(key);
    }
    void removeContext(string key){
        auto ret = context_.erase(key);
        assert(ret == 1);
    }
private:
    std::map<string, void*> context_;
};



#endif// MYWEBSERVER_HTTP_CONTEXTMAP_H