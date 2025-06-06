#include "../http/httpServer.h"
#include "../logging/logger.h"
#include "../common/fileUtil.h"
#include "../mysql/sqlConnectionGuard.h"


using namespace webserver;

string rootDir = detail::FilePath(__FILE__).toString() + "/testResource";
SqlConnectionPool sqlPool(5, "127.0.0.1", "testHttp", "123456", "test_http_database", 3306);//初始化sql连接池

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
void httpCallback(const HttpRequest* request, HttpResponse* response, ContextMap& context){
    TcpConnection* conn = static_cast<TcpConnection*>(context.getContext("CONNECTION"));
    LOG_INFO << ">>>>>>>>>get request from " << conn->peerAddressString() << ":\n" << request->toString();
    auto path = request->getPath();
    response->setVersion(request->getVersion());
    if(request->getMethod() == http::Method::mGET){//获取网页、资源情况。
        if(path == "/username.cgi"){
            bool login = false;
            string username;
            auto cookieMap = http::resolveContentPair(request->getHeaderValue("Cookie"), ';');
            if(cookieMap.find("sessionid") != cookieMap.end()){
                char buf[128] = "SELECT username FROM users WHERE id = (SELECT userid FROM sessions WHERE sessionid = '";
                strcat(buf, cookieMap.at("sessionid").c_str());
                strcat(buf, "');");
                SqlConnectionGuard sqlConn(sqlPool);
                sqlConn.asssertQuery(buf);
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
                    char buf[128] = "DELETE FROM sessions WHERE sessionid = '";
                    strcat(buf, cookieMap.at("sessionid").c_str());
                    strcat(buf, "';");
                    SqlConnectionGuard sqlConn(sqlPool);
                    sqlConn.asssertQuery(buf);//如果没有搜索到会报错吗？
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
                    if(response->getHeaderValue(item) == "image/png"){
                        LOG_INFO << response->toString();
                    }
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
                char buf[256] = "SELECT id, username, password FROM users WHERE username='";
                strcat(buf, pairs.at("username").c_str());
                strcat(buf, "';");
                sqlConn.asssertQuery(buf);
                auto outcome = sqlConn.fetchRow();
                if(outcome && pairs.at("password") == outcome[2]){//登陆成功。首先删除
                    auto sessionid = generateSessionId();
                    char idBuf[64];
                    detail::formatInteger(idBuf, sessionid);

                    //让之前登陆的用户退出。
                    strcpy(buf, "DELETE FROM sessions WHERE userid = '");
                    strcat(buf, outcome[0]);
                    strcat(buf, "';");
                    sqlConn.asssertQuery(buf);

                    //插入新的会话信息。
                    strcpy(buf, "INSERT INTO sessions (userid, sessionid) VALUES (");
                    strcat(buf, outcome[0]);
                    strcat(buf, ", ");
                    strcat(buf, idBuf);
                    strcat(buf, ");");
                    sqlConn.asssertQuery(buf);

                    response->setStatusCode(302);
                    response->setHeaderValue("Location", "/");
                    response->setHeaderValue("Set-Cookie", "sessionid=" + string(idBuf) + "");
                }else{//登陆失败，用户名不存在或者密码不正确。
                    response->setStatusCode(200);
                    string filePath = rootDir + string("/error/loginFailed.html");
                    response->setBodyWithFile(filePath);
                }
            }else if(pairs.at("action") == "signup"){
                char buf[256] = "SELECT username FROM users WHERE username='";
                strcat(buf, pairs.at("username").c_str());
                strcat(buf, "';");
                int ret = sqlConn.query(buf);
                assert(ret == 0);
                auto outcome = sqlConn.fetchRow();
                if(outcome){//注册失败
                    response->setStatusCode(302);
                    response->setHeaderValue("Location", "/error/usernameExists.html");
                }else{//注册成功。实际上插入sql时还是有可能因为竞争而失败，这时应该重定向到错误。只有插入成功后才设定请求。
                    //数据库添加注册信息
                    char buf[256] = "INSERT INTO users (username, password) VALUES (";
                    strcat(buf, pairs.at("username").c_str());
                    strcat(buf, ", ");
                    strcat(buf, pairs.at("password").c_str());
                    strcat(buf, ");");
                    int ret = sqlConn.query(buf);
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

    LOG_INFO << "<<<<<<<send response (omit body):\n" << response->toStringWithoutBody();
}

int main(){
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

        //超过一定时间即清除会话。
        sqlConn.asssertQuery( \
            "CREATE TABLE IF NOT EXISTS sessions("\
            "id INT AUTO_INCREMENT PRIMARY KEY,"\
            "userid VARCHAR(50) NOT NULL UNIQUE," \
            "sessionid VARCHAR(50) NOT NULL UNIQUE,"
            "login_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP" \
            ");" \
        );
    }
    Global::setGlobalLogLevel(Logger::Debug);
    EventLoop baseLoop = EventLoop();

    InetAddress serverAddress = InetAddress("127.0.0.1", 8081, AF_INET);
    HttpServer httpServer(&baseLoop, string_view("server"), serverAddress);
    httpServer.setHttpCallback(httpCallback);
    httpServer.start(4);

    baseLoop.loop();

    return 0;
}