#include "httpServer.h"

using namespace webserver;

void http::defaultHttpCallback(const shared_ptr<TcpConnection>& conn, const HttpRequest* request){
    LOG_INFO << "Get request:\n" << request->toString() << " from conn "<<conn->nameString();
}