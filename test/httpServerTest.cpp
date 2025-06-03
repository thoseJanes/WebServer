#include "../http/httpServer.h"
#include "../logging/logger.h"
#include "../common/fileUtil.h"

using namespace webserver;

string rootDir = "../testResource";
void httpCallback(const shared_ptr<TcpConnection>& conn, const HttpRequest* request){
    //LOG_INFO << request->toString();
    //极其低效的传输方式。
    if(request->getPath() == "/"){
        HttpResponse response(request);
        SmallFileReader fileContent(rootDir + "/desktop/desktop.html", 1024*64*64);
        response.setBody(fileContent.toStringView());
        response.setStatusCode(200);
        LOG_INFO << "response:\n" << response.toStringWithoutBody();//应该只截取头部打印。
        conn->send(response.toString());
    }
}


int main(){
    Global::setGlobalLogLevel(Logger::Debug);
    EventLoop baseLoop = EventLoop();
    InetAddress addr = InetAddress("127.0.0.1", 8080, AF_INET);
    HttpServer httpServer(&baseLoop, string_view("server"), addr);
    httpServer.setHttpCallback(httpCallback);
    httpServer.start(4);

    baseLoop.loop();
}