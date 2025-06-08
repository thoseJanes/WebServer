#include "httpServer.h"

using namespace webserver;

void http::defaultHttpCallback(const HttpRequest* request, HttpResponse* response, ContextMap& context){
    LOG_INFO << "Get request:\n" << request->toString();
}

// void http::defaultHttpCallback(const HttpRequest* request, HttpResponse* response){
//     LOG_INFO << "Get request:\n" << request->toString();
// }