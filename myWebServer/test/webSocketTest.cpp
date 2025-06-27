#include "../websocket/webSocketServer.h"
#include "../../myNetLib/net/tcpClient.h"
#include <any>

using namespace mywebserver;

int main(){
    string str = "test info";
    printf("str:%s\n", str.c_str());

    webSocket::toMasked(str.data(), str.size(), 0x0258);
    printf("str:%s\n", str.c_str());

    webSocket::toUnmasked(str.data(), str.size(), 0x0258, 0);
    printf("str:%s\n", str.c_str());

}