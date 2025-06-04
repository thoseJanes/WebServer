#include "../http/httpServer.h"
#include "../logging/logger.h"
#include "../common/fileUtil.h"
#include "../mysql/sqlConnectionGuard.h"

using namespace webserver;

string rootDir = detail::FilePath(__FILE__).toString() + "/testResource";
SqlConnectionPool sqlPool(5, "127.0.0.1", "testHttp", "123456", "test_http_database", 3306);//初始化sql连接池
map<string, string> urlToPath{
    {"/", "/desktop.html"},
    // {"/desktop.css", "/desktop.css"},
    {"/favicon.ico", "/favicon32.png"},
};

map<string, string> suffixToContentType{
    {"html", "text/html"},
    {"css", "text/css"},
    {"png", "image/png"},
    {"js", "application/x-javascript"},
    {"", "application/octet-stream"}
};

string getContentType(string& path){
    string dir = urlToPath[path];
    auto dotPos = dir.rfind(".");
    if(dotPos != std::string::npos){
        char buf[16];
        auto endPos = std::transform(dir.begin()+dotPos+1, dir.end(), buf, ::tolower);
        *endPos = '\0';
        if(suffixToContentType.find(buf) != suffixToContentType.end()){
            return suffixToContentType.at(buf);
        }
    }
    return suffixToContentType.at("");
}

void httpCallback(const shared_ptr<TcpConnection>& conn, const HttpRequest* request){//应当存储一些临时信息？（目前已经登陆的用户？）如何实现？且由于运行在不同线程中，这个信息的读写必须是能多线程运行的线程安全的共享信息？
    LOG_INFO << "get request from " << conn->peerAddressString() << ":\n" << request->toString();
    HttpResponse response(request);
    auto path = request->getPath();

    //响应资源。

    //固定资源。
    if(request->getMethod() == http::Method::mGET){//获取网页、资源情况。
        if(path == "/username"){

        }

        else{
            bool hasPath = urlToPath.find(path) != urlToPath.end();
            if(hasPath){
                response.setStatusCode(200);
                response.setHeaderValue("Content-Type", getContentType(path));
                SmallFileReader fileContent(rootDir + urlToPath.at(path), 1024*64*64);
                conn->send(response.toStringFromBody(fileContent.toStringView()));
            }else{
                response.setStatusCode(204);
                conn->send(response.toStringWithoutBody());
            }
        }
        
    }else if(request->getMethod() == http::Method::mPOST){//对应不同表单情况。
        if(path == "/userInfo.cgi"){
            //登陆或者将数据加载到sql或者用户不存在。
            
        }else if(path == "/createCharacter.cgi"){
            //收集表单，返回
        }
    }

    LOG_INFO << "send response (omit body):\n" << response.toStringWithoutBody();
}

int main(){
    {
        SqlConnectionGuard sqlConn(sqlPool);
        int ret = sqlConn.query(
            "CREATE TABLE IF NOT EXISTS users("\
            "id INT AUTOINCREMENT PRIMARY KEY,"\
            "username VARCHAR(50) NOT NULL UNIQUE," \
            "password VARCHAR(50) NOT NULL," \
            "created_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP," \
            ");"
        );
        assert(ret == 0);
    }
    

    Global::setGlobalLogLevel(Logger::Debug);
    EventLoop baseLoop = EventLoop();
    InetAddress addr = InetAddress("127.0.0.1", 8081, AF_INET);
    HttpServer httpServer(&baseLoop, string_view("server"), addr);
    httpServer.setHttpCallback(httpCallback);
    httpServer.start(4);

    baseLoop.loop();
}