#include "../http/httpServer.h"
#include "../logging/logger.h"
#include "../common/fileUtil.h"
#include "../mysql/sqlConnectionGuard.h"


using namespace webserver;


string sqlUser = "test_http";
string password = "123456";
string databaseName = "test_http_database";
int sqlPort = 3306;

char httpServerAddress[] = "127.0.0.1";
int httpServerPort = 8000;
int threadNum = 6;

string rootDir = detail::FilePath(__FILE__).toString() + "/testResource";



map<string, string> urlToPath{
    {"/favicon.ico", "/favicon32.png"},
    {"/", "/desktop.html"}
};

size_t generateSessionId(){
    static size_t sessionid;
    sessionid ++;
    return sessionid;
}

/*
class SqlConnectionUtil{
    SqlConnectionUtil(SqlConnectionPool sqlPool);
    


    char buf[1024];
    SqlConnectionPool* sqlPool_;
};
#define SqlConnectionUtil(pool, blocking) error "SqlConnectionUtil should be created on the stack."
*/

//由于传入response，会再复制一遍body，降低效率。或者直接将body设置成文件路径？这里先不管。
void httpCallback(SqlConnectionPool* sqlPoolPtr, const HttpRequest* request, HttpResponse* response, ContextMap& context){
    SqlConnectionPool& sqlPool = *sqlPoolPtr;
    TcpConnection* conn = static_cast<TcpConnection*>(context.getContext("CONNECTION"));
    LOG_DEBUG << ">>>>>>>>>get request from " << conn->peerAddressString() << ":\n" << request->toString();
    auto path = request->getPath();
    response->setVersion(request->getVersion());
    if(request->getMethod() == http::Method::mGET){//获取网页、资源情况。
        if(path == "/username.cgi"){
            bool login = false;
            string username;
            auto cookieMap = http::resolveContentPair(request->getHeaderValue("Cookie"), ';');
            if(cookieMap.find("sessionid") != cookieMap.end()){
                // char buf[128] = "SELECT username FROM users WHERE id = (SELECT userid FROM sessions WHERE sessionid = '";
                // strcat(buf, cookieMap.at("sessionid").c_str());
                // strcat(buf, "');");
                SqlConnectionGuard sqlConn(sqlPool);
                sqlConn.queryStream() << "SELECT username FROM users WHERE id = (SELECT userid FROM sessions WHERE sessionid = '"
                    << cookieMap.at("sessionid").c_str()
                    << "');";
                sqlConn.asssertQuery();
                //sqlConn.asssertQuery(buf);
                auto row = sqlConn.fetchRow();
                if(row){
                    username = row[0];
                    login = true;
                }
            }
            
            if(login){
                response->setStatusCode(200);
                response->setBody("<p>欢迎！用户：" + username + "。<a href='/logout.cgi'>[注销]</a></p>");
            }else{
                string body = "<p>当前还未登陆，请先<a href='/login.html'>[登陆]</a></p>";
                response->setStatusCode(200);
                response->setBody(body);
            }
        }else if(path == "/logout.cgi"){
            // if(context.hasContext("LoginInfo")){
            //     auto info = static_cast<LoginInfo*>(context.getContext("LoginInfo"));
            //     delete info;
            // }

            //删除会话记录。
            auto cookieMap = http::resolveContentPair(request->getHeaderValue("Cookie"), ';');
            if(cookieMap.find("sessionid") != cookieMap.end()){
                {
                    // char buf[128] = "DELETE FROM sessions WHERE sessionid = '";
                    // strcat(buf, cookieMap.at("sessionid").c_str());
                    // strcat(buf, "';");
                    SqlConnectionGuard sqlConn(sqlPool);
                    sqlConn.queryStream() << "DELETE FROM sessions WHERE sessionid = '" 
                                        << cookieMap.at("sessionid").c_str()
                                        << "';";
                    sqlConn.asssertQuery();//如果没有搜索到会报错吗？
                }
            }
            //重定向到开始页面
            response->setStatusCode(302);
            response->setHeaderValue("Location", "/");
        }else{
            bool hasPath = urlToPath.find(path) != urlToPath.end();
            if(hasPath){
                response->setStatusCode(302);
                response->setHeaderValue("Location", urlToPath.at(path));
            }else{
                std::string fullPath = rootDir + path;
                if(detail::fileExists(fullPath)){
                    response->setStatusCode(200);
                    response->setHeaderValue("Content-Type", http::getContentType(path));
                    response->setBodyWithFile(fullPath);

                    string item = "Content-Type";
                }else{
                    response->setStatusCode(204);
                }
            }
        }
    }else if(request->getMethod() == http::Method::mPOST){//对应不同表单情况。
        if(path == "/userInfo.cgi"){
            auto pairs = http::resolveContentPair(request->getBody(), '&');
            //登陆或者将数据加载到sql或者用户不存在。
            //如果已经登陆，应该给出一个错误信息：先注销再登陆？还是直接把登陆界面重定向到desktop？选择后者。
            SqlConnectionGuard sqlConn(sqlPool);
            if(pairs.at("action") == "login"){
                sqlConn.queryStream() << "SELECT id, username, password FROM users WHERE username='"
                                    << pairs.at("username").c_str()
                                    << "';";
                sqlConn.asssertQuery();
                auto outcome = sqlConn.fetchRow();
                if(outcome && pairs.at("password") == outcome[2]){//登陆成功。首先删除
                    auto sessionid = generateSessionId();
                    // char idBuf[64];
                    // detail::formatInteger(idBuf, sessionid);

                    //让之前登陆的用户退出。
                    sqlConn.queryStream() << "DELETE FROM sessions WHERE userid = '" << outcome[0] << "';";
                    sqlConn.asssertQuery();

                    //插入新的会话信息。
                    sqlConn.queryStream() << "INSERT INTO sessions (userid, sessionid) VALUES ("
                                        << outcome[0] << "," << sessionid << ");";
                    sqlConn.asssertQuery();

                    response->setStatusCode(302);
                    response->setHeaderValue("Location", "/");
                    response->setHeaderValue("Set-Cookie", "sessionid=" + to_string(sessionid) + "");
                }else{//登陆失败，用户名不存在或者密码不正确。
                    response->setStatusCode(200);
                    string filePath = rootDir + string("/error/loginFailed.html");
                    response->setBodyWithFile(filePath);
                }
            }else if(pairs.at("action") == "signup"){
                // char buf[256] = "SELECT username FROM users WHERE username='";
                // strcat(buf, pairs.at("username").c_str());
                // strcat(buf, "';");
                sqlConn.queryStream() << "SELECT username FROM users WHERE username='"
                                    << pairs.at("username").c_str() << ";";
                sqlConn.asssertQuery();
                auto outcome = sqlConn.fetchRow();
                if(outcome){//注册失败
                    response->setStatusCode(302);
                    response->setHeaderValue("Location", "/error/usernameExists.html");
                }else{//注册成功。实际上插入sql时还是有可能因为竞争而失败，这时应该重定向到错误。只有插入成功后才设定请求。
                    //数据库添加注册信息
                    sqlConn.queryStream() << "INSERT INTO users (username, password) VALUES ("
                                        << pairs.at("username").c_str()
                                        << "," << pairs.at("password").c_str() << ");";
                    int ret = sqlConn.query();
                    if(ret){
                        response->setStatusCode(302);
                        response->setHeaderValue("Location", "/error/usernameExists.html");
                    }else{
                        response->setStatusCode(200);
                        string filePath = rootDir + string("/registeredsuccessfully.html");
                        response->setBodyWithFile(filePath);
                    }
                }
            }
        }else if(path == "/createCharacter.cgi"){
            response->setStatusCode(301);
            response->setHeaderValue("Location", "https://github.com/thoseJanes/webServer");
        }else{
            response->setStatusCode(204);
        }
    }

    LOG_DEBUG << "<<<<<<<send response (omit body):\n" << response->toStringWithoutBody();
}

int main(int argc, char* argv[]){
    Global::setGlobalLogLevel(Logger::Warn);
    // if(argc < 3){
    //     LOG_FATAL << "Usage: " << argv[0] << " ipv4_address ip_port number_of_threads";
    // }
    // char* httpServerAddress = argv[1];
    // int httpServerPort = atoi(argv[2]);
    // int threadNum = atoi(argv[3]);
    SqlUser user = {"127.0.0.1", sqlUser.c_str(), password.c_str()};
    SqlConnectionPool sqlPool(threadNum, user, databaseName.c_str(), sqlPort);//初始化sql连接池s
    {//初始化要用到的数据表。
        SqlConnectionGuard sqlConn(sqlPool);
        sqlConn.asssertQuery( \
            "CREATE TABLE IF NOT EXISTS users("\
            "id INT AUTO_INCREMENT PRIMARY KEY,"\
            "username VARCHAR(50) NOT NULL UNIQUE," \
            "password VARCHAR(50) NOT NULL," \
            "created_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP" \
            ");" \
        );

        sqlConn.asssertQuery( \
            "CREATE TABLE IF NOT EXISTS sessions("\
            "id INT AUTO_INCREMENT PRIMARY KEY,"\
            "userid VARCHAR(50) NOT NULL UNIQUE," \
            "sessionid VARCHAR(50) NOT NULL UNIQUE,"
            "login_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP" \
            ");" \
        );
    }
    
    EventLoop baseLoop = EventLoop();

    InetAddress serverAddress = InetAddress(httpServerAddress, httpServerPort, AF_INET);
    HttpServer httpServer(&baseLoop, string_view("server"), serverAddress);
    httpServer.setHttpCallback(bind(&httpCallback, &sqlPool, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    httpServer.start(threadNum);

    baseLoop.loop();

    return 0;
}