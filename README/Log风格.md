# Log风格
打印格式
----

```text-plain
[类名::函数] - [事件及相关信息]//有时候不打印身份信息（[类名::函数]），或只打印类名。
[类名::函数] - [errorType] = [errno] [strError]
ctor[指针]//其实可以加上类名？
[类名::函数]//LOG_SYS相关打印只给出错误发生的函数名信息，错误类型会自动打印。除非存在多种需要区分的错误类型，如可以预料的错误和没有预料的错误
```

其中

```text-plain
[类名::函数]是身份信息，如果实例有name属性，可以一并打印：[类名::函数]\[name\]
```

可以适当加入<>、\[\]、{}、()等符号来强调重点，使log更清晰。

LOG分级
-----

LOG分级貌似并不是很清晰，在代码不同部分貌似风格不一致（如accept和connect中）。能确定的貌似只有ERROR和FATAL相关的打印方式及LOG和LOG\_SYS的区别

*   会设置errno的错误，使用LOG\_SYS，否则使用LOG\_
*   错误需要退出用FATAL，不退出用ERROR

但是可以分出一些特定的常见打印位置：

*   初始全局设置
*   构造析构信息
*   连接建立与断开、尝试监听/连接
*   特定功能（connector的retry功能、acceptor释放占位描述符）

muduo源码打印内容
-----------

Channel：

LOG\_WARN打印fd及其错误类型

LOG\_TRACE打印接收到的事件

Connector：

构造和析构中LOG\_DEBUG打印`ctor[指针]`、`dtor[指针]`

LOG\_DEBUG打印被stop截停的startInLoop和retry，`do not connect`

LOG\_SYSERR打印connect发生的错误，分为`connect error in [类名::函数]`、`Unexpected error in [类名::函数]`

handleWrite中，LOG\_TRACE打印`[类名::函数] - state`，LOG\_WARN打印创建Channel后的失败情况`[类名::函数] - SO_ERROR = [errno] [strerror]`

handleError中，LOG\_TRACE打印失败情况（格式类似。）

LOG\_INFO打印retry提醒：`[类名::函数] - retry connecting to [ip::port] in [n] milliseconds`

EventLoop：

LOG\_SYSERR打印`Failed in [函数]`

IgnoreSIGPIPE中LOG\_TRACE打印`Ignore SIGPIPE`。（即，初始化一些全局设置时，把这些设置打印出来以便调试。感觉可以用更高日志级别，反正这种设置只会被设置一遍。）

EventLoop的构造析构打印。LOG\_DEBUG打印`EventLoop created [指针] in thread [tid]`、LOG\_DEBUG打印`EventLoop [指针] of thread [tid] destruct in thread [tid]`，感觉EventLoop的构造析构打印没有connector简洁，可以优化

LOG\_TRACE打印`EventLoop [pointer] start looping`、`EventLoop [pointer] stop looping`

LOG\_FATAL打印`Another EventLoop [指针] exists in this thread [tid]`（属于错误情况打印，说明错误类型）

assertInLoop失败时，打印`[类::函数] - [哪个线程创建、在哪个线程assert]`（错误情况打印）

eventfd写入或读取非64位时，LOG\_ERROR，打印`EventLoop::wakeup() writes/reads [n] bytes instead of 8`（错误情况打印）

LOG\_TRACE打印ActiveChannels，在channel中设置格式化函数，只打印每个channel的fd和revent

InetAddress：

LOG\_SYSERR报告地址解析时的错误。`[类名::函数]`

Socket：

LOG\_SYSERR设置socket选项失败时，打印设置的选项名`[选项名] failed`。（注意选项未定义的情况）

LOG\_SYSERR打印各种包装系统调用函数的失败情况，只给出`[namespace::funcName]`

accept的打印方式类似connect。

TcpClient：

LOG\_INFO打印TcpClient创建的connector的指针。应该是因为connector没有名字，但是connector内部通过connector指针打印相关信息，需要知道connector属于哪个TcpClient

connect或重新retry时，LOG\_INFO打印`[类名::函数]\[客户名\] - connecting/reconnecting to [ip::port]`，其中客户名属于身份信息。

析构时，打印`TcpClient::~TcpClient\[name\] - connector [pointer]`，前者是身份信息，后面是标识connect。可能是想观察TcpClient和Connect的析构顺序是否一致？

后面简单看了一下，基本都是这个模式。