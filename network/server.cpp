#include <iostream>

#include "socket.h"

using namespace network::socket;

int main()
{

    // 1. 创建 socket
    Socket server;

    // 2. 绑定 socket
    server.bind("127.0.0.1", 8080);

    // 3. 监听 socket
    server.listen(1024);

    while (true)
    {
        // 4. 接收客户端连接
        int connfd = server.accept();
        if (connfd < 0)
        {
            return 1;
        }

        Socket client(connfd);

        char buf[1024] = {0};

        // 5. 接收客户端的数据
        size_t len = client.recv(buf, sizeof(buf));
        printf("recv: connfd=%d msg=%s\n", connfd, buf);

        // 6. 向客户端发送数据
        client.send(buf, len);
    }

    // 7. 关闭 socket
    server.close();
    return 0;
}