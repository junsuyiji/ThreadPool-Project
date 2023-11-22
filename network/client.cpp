#include <iostream>

#include "socket.h"
using namespace network::socket;

int main()
{
    // 1. 创建 socket
    Socket client;

    // 2. 连接服务端
    client.connect("127.0.0.1", 8080);

    // 3. 向服务端发送数据
    string data = "hello world";
    client.send(data.c_str(), data.size());

    // 4. 接收服务端的数据
    char buf[1024] = {0};
    client.recv(buf, sizeof(buf));
    printf("recv: %s\n", buf);

    // 5. 关闭 socket
    client.close();

    return 0;
}