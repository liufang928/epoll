#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include"message.hpp"

#define IP "127.0.0.1"
#define PORT 8080

class Client
{
private:
    int cfd;
    std::string username;

public:
    Client();
    ~Client();
    void Send(std::string msg);
    void Send(Header* msg);
};

Client::Client()
{
    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1)
    {
        std::cerr << "Failed to create socket." << std::endl;
        return;
    }
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(IP); // 服务器IP地址
    server_addr.sin_port = htons(PORT);          // 服务器端口号

    // 连接到服务器
    if (connect(cfd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        std::cerr << "Failed to connect to server." << std::endl;
        close(cfd);
        return;
    }

    // 设置socket为非阻塞模式
    int flags = fcntl(cfd, F_GETFL, 0);
    if (flags == -1)
    {
        std::cerr << "Failed to get socket flags." << std::endl;
        close(cfd);
        return;
    }
    if (fcntl(cfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        std::cerr << "Failed to set socket to non-blocking." << std::endl;
        close(cfd);
        return;
    }
}
Client::~Client()
{
    if (cfd != -1)
    {
        close(cfd);
    }
}

void Client::Send(std::string msg)
{
    char *buff = new char[msg.length() + 1];
    strcpy(buff, msg.c_str());
    int result = send(cfd, buff, strlen(buff), 0);
    std::cerr << "send :" << msg << "   ---->   result : " << result << std::endl;
}
void Client::Send(Header* msg)
{
    int result = send(cfd, (const char*)msg, msg->len, 0);
    std::cerr << "send :" << MsgType::ToString(msg->type) << "   ---->   result : " << result << std::endl;
}

int main()
{
    Client client;

    MessageLogin login;
    strcpy(login.username,"zsh");
    strcpy(login.password,"123x");

    for (int i = 0; i < 1000000; i++)
    {
        client.Send(&login);
    }
}