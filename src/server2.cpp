#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <fcntl.h>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <memory>

#include "utils/util.hpp"
#include "message.hpp"
#include "ThreadPool.hpp"

#define PORT 8080

class Server
{
private:
    std::unordered_map<int, int> clients;

private:
    bool isDone;
    int sfd;
    int epfd;
    sockaddr_in server_addr;
    epoll_event events[10];
    std::atomic<int> clientCount;
    std::atomic<int> recvCount;
    std::atomic<int> totalRecvCount;
    double totalTime;
    Timer timer;
    ThreadPool pool;

    char buffer[10240];
    int lastPos = 0;
    char buffer2[10240 * 10];

public:
    Server();
    ~Server();
    void Run();
    void ProcessMsg(const char *buffer, int count);
    void time4msg();
};

void Server::time4msg()
{
    while (!isDone)
    {
        double time = timer.getSecond();
        if (time >= 1.0)
        {
            totalTime += time;
            std::cerr << "recv clients[" << clients.size() << "]   time[" << time << "]    avg[" << recvCount / time << "]" << "    total[" << totalRecvCount << "]    useTime[" << totalTime << "]" << std::endl;
            recvCount = 0;
            timer.update();
        }
    }
}

void Server::ProcessMsg(const char *buffer, int count)
{
    // 粘包 少包
    memcpy(buffer2 + lastPos, buffer, count);
    lastPos += count;
    while (lastPos >= sizeof(Header))
    {
        Header *header = (Header *)buffer2;
        if (lastPos >= header->len)
        {
            int n_size = lastPos - header->len;
            if (MsgType::ToString(header->type) == "LOGIN")
            {
                // MessageLogin *login = (MessageLogin *)header;
                // std::cerr << "recv data [" << login->username << "] [" << login->password << "]" << std::endl;
            }
            else
            {
                std::cerr << "unknow msg type" << std::endl;
            }
            memcpy(buffer2, buffer2 + header->len, n_size);
            lastPos = n_size;
        }
        else
        {
            //std::cerr << "消息长度不够" << std::endl;
        }
    }
}
void Server::Run()
{
    std::thread t(&Server::time4msg, this);
    t.detach();

    while (!isDone)
    {
        int num_events = epoll_wait(epfd, events, 10, -1);
        if (num_events == -1)
        {
            std::cerr << "Failed to wait for events." << std::endl;
            break;
        }

        for (int i = 0; i < num_events; i++)
        {
            if (events[i].data.fd == sfd)
            {
                // 接受新的连接
                sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                int client_fd = accept(sfd, (sockaddr *)&client_addr, &addr_len);
                if (client_fd == -1)
                {
                    std::cerr << "Failed to accept connection." << std::endl;
                    continue;
                }
                // 设置client_fd为非阻塞模式
                int flags = fcntl(client_fd, F_GETFL, 0);
                if (flags == -1)
                {
                    std::cerr << "Failed to get socket flags." << std::endl;
                    close(client_fd);
                    continue;
                }
                if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1)
                {
                    std::cerr << "Failed to set socket to non-blocking." << std::endl;
                    close(client_fd);
                    continue;
                }
                // 注册client_fd到epoll实例
                epoll_event event;
                event.data.fd = client_fd;
                // event.events = EPOLLIN | EPOLLET;
                event.events = EPOLLIN;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &event) == -1)
                {
                    std::cerr << "Failed to add client_fd to epoll." << std::endl;
                    close(client_fd);
                    continue;
                }
                clientCount++;
                clients[client_fd] = 1;
                // std::cerr << "count[" << clientCount << "]" << std::endl;
            }
            else
            {
                ssize_t count = recv(events[i].data.fd, buffer, sizeof(buffer),0);
                if (count == -1)
                {
                    std::cerr << "Failed to read from socket." << std::endl;
                }
                else if (count == 0)
                {
                    // 客户端关闭了连接
                    close(events[i].data.fd);
                    clients.erase(events[i].data.fd);
                    // std::cerr << "client[" << events[i].data.fd << "] closed!" << std::endl;
                }
                else
                {
                    recvCount++;
                    totalRecvCount++;
                    //pool.enqueue(&Server::ProcessMsg, this, buffer, count);
                    ProcessMsg(buffer,count);
                }
            }
        }
    }
}

Server::Server() : clientCount(0), recvCount(0), totalRecvCount(0), totalTime(0.0), pool(4)
{
    isDone = false;
    sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sfd == -1)
    {
        std::cerr << "Failed to create socket." << std::endl;
        exit(1);
    }
    // 设置服务器地址
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定socket
    if (bind(sfd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        std::cerr << "Failed to bind socket." << std::endl;
        close(sfd);
        exit(1);
    }

    // 监听socket
    if (listen(sfd, SOMAXCONN) == -1)
    {
        std::cerr << "Failed to listen on socket." << std::endl;
        close(sfd);
        exit(1);
    }

    // 创建epoll实例
    epfd = epoll_create1(0);
    if (epfd == -1)
    {
        std::cerr << "Failed to create epoll instance." << std::endl;
        close(sfd);
        exit(1);
    }

    // 注册server_fd到epoll实例
    epoll_event event;
    event.data.fd = sfd;
    event.events = EPOLLIN;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &event) == -1)
    {
        std::cerr << "Failed to add server_fd to epoll." << std::endl;
        close(sfd);
        close(epfd);
        exit(1);
    }

    //初始化
    memset(buffer,0,sizeof(buffer));
    memset(buffer2,0,sizeof(buffer2));
}

Server::~Server()
{
    isDone = true;
}

int main()
{
    Server server;
    server.Run();
}