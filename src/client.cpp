#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <string>
#include "message.hpp"

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
    void Connect();
    void Send(std::string msg);
    void Send(Header *msg,int count);
};

void Client::Connect()
{
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(IP); // 服务器IP地址
    server_addr.sin_port = htons(PORT);          // 服务器端口号

    // 连接到服务器
    if (connect(cfd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        std::cerr << cfd <<" -- Failed to connect to server." << std::endl;
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

Client::Client()
{
    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1)
    {
        std::cerr << "Failed to create socket." << std::endl;
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
void Client::Send(Header *header,int count)
{
    send(cfd, (const char*)header, count, 0);
}

#define CLIENT_SIZE 4000
Client *clients[CLIENT_SIZE];
void test_single()
{
    for (int i = 0; i < CLIENT_SIZE; i++)
    {
        clients[i] = new Client();
    }
    for (int i = 0; i < CLIENT_SIZE; i++)
    {
        clients[i]->Connect();
    }
    std::cerr << "client connect completed!" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    MessageLogin login;
    strcpy(login.username, "zsh");
    strcpy(login.password, "123x");
    for (int j = 0; j < 250; j++)
    {
        for (int i = 0; i < CLIENT_SIZE; i++)
        {
            clients[i]->Send(&login,login.len);
        }
    }
    std::cerr << "send msg finished!" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    for (int i = 0; i < CLIENT_SIZE; i++)
    {
        if (clients[i] != nullptr)
            delete clients[i];
    }
}

void test_threads(int id)
{
    int c = CLIENT_SIZE / 4;
    int begin = (id - 1) * c;
    int end = id * c;

    for (int i = begin; i < end; i++)
    {
        clients[i] = new Client();
    }
    for (int i = begin; i < end; i++)
    {
        clients[i]->Connect();
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    MessageLogin login;
    strcpy(login.username, "zsh");
    strcpy(login.password, "123x");
    for (int i = 0; i < 25; i++)
    {
        for (int i = begin; i < end; i++)
        {
            clients[i]->Send(&login,login.len);
        }
    }
}

int main()
{
    for (int i = 1; i <= 4; i++)
    {
        std::thread t(test_threads, i);
        t.detach();
    }

    std::this_thread::sleep_for(std::chrono::seconds(20));
    for (int i = 0; i < CLIENT_SIZE; i++)
    {
        if (clients[i] != nullptr)
            delete clients[i];
    }

   // test_single();
}