#include "httpServer.h"

using namespace mywebserver;

void http::defaultHttpCallback(const HttpRequest* request, HttpResponse* response, ContextMap& context){
    LOG_INFO << "Get request:\n" << request->toString();
}

// void http::defaultHttpCallback(const HttpRequest* request, HttpResponse* response){
//     LOG_INFO << "Get request:\n" << request->toString();
// }