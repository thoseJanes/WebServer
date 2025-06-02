#include "httpServer.h"

using namespace webserver;

void http::defaultHttpCallback(const HttpRequest* request){
    LOG_INFO << "Get request:\n" << request->toString();
}