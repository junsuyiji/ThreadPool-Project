//socket.cpp

#include <sys/socket.h>
#include "socket.h"
#include <string.h>

using namespace network::socket;

Socket::Socket() : m_ip(""), m_port(0), m_sockfd(0)
{
    // 1. 创建 socket
    m_sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sockfd < 0)
    {
        printf("create socket error: errno=%d errmsg=%s\n", errno, strerror(errno));
    }
    printf("socket constructed\n");
}

Socket::Socket(int sockfd): m_ip(""), m_port(0), m_sockfd(sockfd)
{
    printf("socket constructed canshu\n");
}

Socket::~Socket()
{
    close();
}

bool Socket::bind(const string & ip, int port)
{
    // 2. 绑定 socket

    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    if (ip.empty())
    {
        sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        sockaddr.sin_addr.s_addr = inet_addr(ip.c_str());
    }
    sockaddr.sin_addr.s_addr = inet_addr(ip.c_str());
    sockaddr.sin_port = htons(port);
    if (::bind(m_sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
    {
        printf("socket bind error: errno=%d, errmsg=%s\n", errno, strerror(errno));
        return false;
    }
    m_ip = ip;
    m_port = port;
    
    printf("socket bind success: ip=%s port=%d\n", ip.c_str(), port);
    
    return true;
}

bool Socket::listen(int backlog)
{
    // 3. 监听 socket
    if (::listen(m_sockfd, backlog) < 0)
    {
        printf("socket listen error: errno=%d errmsg=%s\n", errno, strerror(errno));
        return false;
    }
    
    
    printf("socket listen ...\n");
    
    return true;
}

bool Socket::connect(const string & ip, int port)
{
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(ip.c_str());
    sockaddr.sin_port = htons(port);
    if (::connect(m_sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
    {
        printf("socket connect error: errno=%d errmsg=%s\n", errno, strerror(errno));
        return false;
    }

    m_ip = ip;
    m_port = port;
    return true;
}

int Socket::accept()
{
    // 4. 接收客户端连接
    int connfd = ::accept(m_sockfd,nullptr, nullptr);
    if (connfd < 0)
    {
        printf("socket accept error: errno=%d errmsg=%s\n", errno, strerror(errno));
        return false;
    }

    return connfd;
}

int Socket::send(const char *buf, int len)
{
    return ::send(m_sockfd, buf, len, 0);
}

int Socket::recv(char *buf, int len)
{
    return ::recv(m_sockfd, buf, len, 0);
}

void Socket::close()
{
    if (m_sockfd > 0)
    {
        ::close(m_sockfd);
        m_sockfd = 0; 
    }
}