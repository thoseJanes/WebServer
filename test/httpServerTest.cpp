#include <hiredis/hiredis.h>
#include <hiredis/read.h>

#include "../http/httpServer.h"
#include "../logging/logger.h"
#include "../common/fileUtil.h"
#include "../mysql/sqlConnectionGuard.h"
#include "../redis/redisConnectionGuard.h"

using namespace webserver;
string sqlUser = "test_http";
string password = "123456";
string databaseName = "test_http_database";
int sqlPort = 3306;
int redisPort = 6379;

char httpServerAddress[] = "127.0.0.1";
int httpServerPort = 8000;
int threadNum = 6;

string rootDir = detail::FilePath(__FILE__).toString() + "/testResource";

SqlConnectionPool* sqlPool;
RedisConnectionPool* redisPool;
void initConnectionPool(){
    Global::setGlobalLogLevel(Logger::Warn);
    mysql::SqlServer sqlServer = {sqlUser.c_str(), password.c_str(), databaseName.c_str(), "127.0.0.1", sqlPort};
    sqlPool = new SqlConnectionPool(threadNum, sqlServer, mysql::createMysqlConnection, mysql::destroyMysqlConnection);
    redis::RedisServer redisServer = {"127.0.0.1", redisPort};
    redisPool = new RedisConnectionPool(threadNum, redisServer, redis::createRedisConnection, redis::destroyRedisConnection);
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
    {//清空数据库数据？
        RedisConnectionGuard redisConn(redisPool);
    }
}

map<string, string> redirectPath{
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

void processStaticPage(string_view fullPath, HttpResponse* response){
    if(detail::fileExists(fullPath)){
        response->setStatusCode(200);
        response->setHeaderValue("Content-Type", http::getContentType(fullPath));
        response->setBodyWithFile(fullPath);
    }else{
        response->setStatusCode(204);
    }
}

void processUsernameCgi(const HttpRequest* request, HttpResponse* response){
    bool login = false;
    string username;
    auto cookieMap = http::resolveContentPair(request->getHeaderValue("Cookie"), ';');
    if(cookieMap.find("sessionid") != cookieMap.end()){
        // SqlConnectionGuard sqlConn(sqlPool);
        // sqlConn.queryStream() << "SELECT username FROM users WHERE id = (SELECT userid FROM sessions WHERE sessionid = '"
        //     << cookieMap.at("sessionid").c_str()
        //     << "');";
        // sqlConn.asssertQuery();
        // //sqlConn.asssertQuery(buf);
        // auto row = sqlConn.fetchRow();
        // if(row){
        //     username = row[0];
        //     login = true;
        // }
        RedisConnectionGuard redisConn(redisPool);
        auto reply = redisConn.freeReplyAndCommand("hget session:%s username", cookieMap.at("sessionid").c_str());
        if(reply->type != REDIS_REPLY_NIL){//已登陆
            assert(reply->type == REDIS_REPLY_STRING);
            login = true;
            username = reply->str;
        }
    }

    if(login){
        response->setStatusCode(200);
        response->setBody("<p>欢迎！用户：" + username + "。<a href='/logout.cgi'>[注销]</a></p>");
    }else{
        string_view body = "<p>当前还未登陆，请先<a href='/login.html'>[登陆]</a></p>";
        response->setStatusCode(200);
        response->setBody(body);
    }
}

void processLogOut(const HttpRequest* request, HttpResponse* response){
    //删除会话记录。
    auto cookieMap = http::resolveContentPair(request->getHeaderValue("Cookie"), ';');
    if(cookieMap.find("sessionid") == cookieMap.end()){
        response->redirectTo("/");
        return;
    }

    // char buf[128] = "DELETE FROM sessions WHERE sessionid = '";
    // strcat(buf, cookieMap.at("sessionid").c_str());
    // strcat(buf, "';");

    // SqlConnectionGuard sqlConn(sqlPool);
    // sqlConn.queryStream() << "DELETE FROM sessions WHERE sessionid = '" 
    //                     << cookieMap.at("sessionid").c_str()
    //                     << "';";
    // sqlConn.asssertQuery();//如果没有搜索到会报错吗？
    RedisConnectionGuard redisConn(redisPool); redisReply* reply;
    reply = redisConn.freeReplyAndCommand("hget session:%s userid", cookieMap.at("sessionid").c_str());
    if(reply->type == REDIS_REPLY_NIL){
        LOG_WARN << "httpCallback: logout - Session id not exists.";
        response->redirectTo("/");
        return;
    }

    string userid = reply->str;
    reply = redisConn.freeReplyAndCommand("MULTI");
    if(REDIS_REPLY_STATUS != reply->type){//要确保每条都删除成功了?其实删除不成功也没关系，顶多内存泄漏，可以在登陆时清理。
        LOG_WARN << "httpCallback: logout - Failed in redis command MULTI."
        <<" Session "<< cookieMap.at("sessionid").c_str() <<" info has not been deleted.";
        response->redirectTo("/");
        return;
    }

    redisConn.appendCommand("del session:%s", cookieMap.at("sessionid").c_str());
    redisConn.appendCommand("hdel activeUsers %s", userid.c_str());
    redisConn.appendCommand("EXEC");
    reply = redisConn.getTheLastReply();

    //重定向到开始页面
    response->redirectTo("/");
}

void processLogIn(const map<string,string>& pairs, HttpResponse* response){
    SqlConnectionGuard sqlConn(sqlPool);
    sqlConn.queryStream() << "SELECT id, username, password FROM users WHERE username='"
                                    << pairs.at("username").c_str()
                                    << "';";
    sqlConn.asssertQuery();
    auto outcome = sqlConn.fetchRow();
    if((!outcome) || pairs.at("password") != outcome[2]){//用户名或者密码错误
        response->setStatusCode(200);
        string filePath = rootDir + string("/error/loginFailed.html");
        response->setBodyWithFile(filePath);
        return;
    }

    auto sessionid = generateSessionId();
    //需要知道当前有没有人登陆同一个帐号。让之前登陆的用户退出。其实也可以直接把sessionid设置成userid来简化操作？但是删除用户时可能容易串话？
    RedisConnectionGuard redisConn(redisPool); redisReply* reply;
    reply = redisConn.freeReplyAndCommand("hget activeUsers %s", outcome[0]);
    string existsSession;
    if(reply->type != REDIS_REPLY_NIL){
        existsSession = reply->str;//也有可能发生错误？
    }

    //添加信息。
    reply = redisConn.freeReplyAndCommand("MULTI");
    if(REDIS_REPLY_STATUS != reply->type){//开始事务失败
        response->setStatusCode(200);
        string filePath = rootDir + string("/error/loginFailedServerError.html");
        response->setBodyWithFile(filePath);
        return;
    }
    if(!existsSession.empty()){
        redisConn.appendCommand("hdel activeUsers %s", outcome[0]);//删除不存在的键不会导致失败
        redisConn.appendCommand("del session %s", existsSession.c_str());//删除之前的整个会话（或许应该给之前的会话发送一个下线提示？）
    }
    redisConn.appendCommand("hset session:%d username %s userid %s", sessionid, outcome[1], outcome[0]);
    redisConn.appendCommand("hset activeUsers %s %d", outcome[0], sessionid);
    redisConn.appendCommand("EXEC");
    reply = redisConn.getTheLastReply();
    if(REDIS_REPLY_ARRAY != reply->type){//执行事务失败
        response->setStatusCode(200);
        string filePath = rootDir + string("/error/loginFailedServerError.html");
        response->setBodyWithFile(filePath);
        return;
    }

    response->setStatusCode(302);
    response->setHeaderValue("Location", "/");
    response->setHeaderValue("Set-Cookie", "sessionid=" + to_string(sessionid) + "");
}

void processSignUp(const map<string,string>& pairs, HttpResponse* response){
    SqlConnectionGuard sqlConn(sqlPool);
    // char buf[256] = "SELECT username FROM users WHERE username='";
    // strcat(buf, pairs.at("username").c_str());
    // strcat(buf, "';");
    sqlConn.queryStream() << "SELECT username FROM users WHERE username='"
                        << pairs.at("username").c_str() << "';";
    sqlConn.asssertQuery();
    auto outcome = sqlConn.fetchRow();
    if(outcome){//已经存在用户名，注册失败
        response->setStatusCode(302);
        response->setHeaderValue("Location", "/error/usernameExists.html");
        return;
    }
    //注册成功。实际上插入sql时还是有可能因为竞争而失败，这时应该重定向到错误。只有插入成功后才设定请求。
    //数据库添加注册信息
    sqlConn.queryStream() << "INSERT INTO users (username, password) VALUES ("
                        << pairs.at("username").c_str()
                        << "," << pairs.at("password").c_str() << ");";
    int ret = sqlConn.query();
    if(ret){
        response->setStatusCode(302);
        response->setHeaderValue("Location", "/error/usernameExists.html");
        return;
    }

    response->setStatusCode(200);
    string filePath = rootDir + string("/registeredsuccessfully.html");
    response->setBodyWithFile(filePath);
}

void processGame(const HttpRequest* request, HttpResponse* response){
    auto cookieMap = http::resolveContentPair(request->getHeaderValue("Cookie"), ';');
    //太多ifelse嵌套了。该怎么办？
    if(cookieMap.find("sessionid") == cookieMap.end()){
        response->setStatusCode(401);
        return;
    }

    RedisConnectionGuard redisConn(redisPool); redisReply* reply;
    reply = redisConn.freeReplyAndCommand("hget session:%s userid", cookieMap.at("sessionid").c_str());
    if(reply->type == REDIS_REPLY_NIL){
        response->setStatusCode(401);
        return;
    }

    response->setStatusCode(301);
    response->setHeaderValue("Location", "https://github.com/thoseJanes/webServer");
}

//由于传入response，会再复制一遍body，降低效率。或者直接将body设置成文件路径？这里先不管。
void httpCallback(const HttpRequest* request, HttpResponse* response, ContextMap& context){
    TcpConnection* conn = static_cast<TcpConnection*>(context.getContext("CONNECTION"));
    LOG_DEBUG << ">>>>>>>>>get request from " << conn->peerAddressString() << ":\n" << request->toString();
    auto path = request->getPath();
    response->setVersion(request->getVersion());
    if(request->getMethod() == http::Method::mGET){//获取网页、资源情况。
        if(path == "/username.cgi"){
            processUsernameCgi(request, response);
        }else if(path == "/logout.cgi"){
            processLogOut(request, response);
        }else if(redirectPath.find(path) != redirectPath.end()){
            response->redirectTo(redirectPath.at(path));
        }else{
            // std::string fullPath = rootDir + path;
            processStaticPage(rootDir + path, response);
        }
    }else if(request->getMethod() == http::Method::mPOST){//对应不同表单情况。
        if(path == "/userInfo.cgi"){
            auto pairs = http::resolveContentPair(request->getBody(), '&');
            //登陆或者将数据加载到sql或者用户不存在。
            //如果已经登陆，应该给出一个错误信息：先注销再登陆？还是直接把登陆界面重定向到desktop？选择后者。
            if(pairs.at("action") == "login"){
                processLogIn(pairs, response);
            }else if(pairs.at("action") == "signup"){
                processSignUp(pairs, response);
            }
        }else if(path == "/createCharacter.cgi"){
            processGame(request, response);
        }else{
            response->setStatusCode(204);
        }
    }

    LOG_DEBUG << "<<<<<<<send response (omit body):\n" << response->toStringWithoutBody();
}

int main(int argc, char* argv[]){
    initConnectionPool();

    EventLoop baseLoop = EventLoop();

    InetAddress serverAddress = InetAddress(httpServerAddress, httpServerPort, AF_INET);
    HttpServer httpServer(&baseLoop, string_view("server"), serverAddress);
    httpServer.setHttpCallback(bind(&httpCallback, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    httpServer.start(threadNum);

    baseLoop.loop();

    return 0;
}