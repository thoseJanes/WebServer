#include "httpResponse.h"

using namespace webserver;
const map<int, string> http::StatusCodeToExplain = {
        {200, "OK"}, // 请求正常处理完毕
        {204, "No Content"}, // 请求正常处理完毕，但响应信息中没有响应正文
        {206, "Partial Content"}, // 请求正常处理完毕，客户端对服务器进行了范围请求，响应报文中包含由Content-Range指定的实体内容范围
        {301, "Moved Permanently"}, // 永久性重定向：请求的资源已经被分配了新的URI，以后应使用新的URI，也就是说，如果之前将老的URI保存为书签了，后面应该按照响应的Location首部字段重新保存书签
        {302, "Found"}, // 临时重定向：目标资源被分配了新的URI，希望用户本次使用新的URI进行访问
        {307, "Temporary Redirect"}, // 临时重定向：目标资源被分配了新的URI，希望用户本次使用新的URI进行访问
        {400, "Bad Request"}, // 请求报文中存在语法错误，需修改请求内容重新发送（浏览器会像200 OK一样对待该状态码）
        {403, "Forbidden"}, // 浏览器所请求的资源被服务器拒绝了。服务器没有必要给出详细的理由，如果想要说明，可以在响应实体内部进行说明
        {404, "Not Found"}, // 浏览器所请求的资源不存在
        {500, "Internal Server Error"}, // 服务器端在执行的时候发生了错误，可能是Web本身存在的bug或者临时故障
        {503, "Server Unavailable"}, // 服务器目前处于超负荷或正在进行停机维护状态，目前无法处理请求。这种情况下，最好写入Retry-After首部字段再返回给客户端
    };