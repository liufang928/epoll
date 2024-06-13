#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <atomic>
#include "message.hpp"


#define PORT 8080
std::atomic<int> recvCount(1);  //统计收到的包数量

int main()
{
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }

    // 设置服务器地址
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定socket
    if (bind(sfd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Failed to bind socket." << std::endl;
        close(sfd);
        return 1;
    }

    // 监听socket
    if (listen(sfd, SOMAXCONN) == -1) {
        std::cerr << "Failed to listen on socket." << std::endl;
        close(sfd);
        return 1;
    }

    // 创建epoll实例
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "Failed to create epoll instance." << std::endl;
        close(epoll_fd);
        return 1;
    }

    // 注册server_fd到epoll实例
    epoll_event event;
    event.data.fd = sfd;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sfd, &event) == -1) {
        std::cerr << "Failed to add server_fd to epoll." << std::endl;
        close(sfd);
        close(epoll_fd);
        return 1;
    }
    // 用于epoll_wait的事件数组
    epoll_event events[10];

    // 主事件循环
    while (true) {
        int num_events = epoll_wait(epoll_fd, events, 10, -1);
        if (num_events == -1) {
            std::cerr << "Failed to wait for events." << std::endl;
            break;
        }

        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == sfd) {
                // 接受新连接
                sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int client_fd = accept(sfd, (sockaddr*)&client_addr, &client_addr_len);
                if (client_fd == -1) {
                    std::cerr << "Failed to accept connection." << std::endl;
                    continue;
                }
                // 设置client_fd为非阻塞模式
                int flags = fcntl(client_fd, F_GETFL, 0);
                if (flags == -1) {
                    std::cerr << "Failed to get socket flags." << std::endl;
                    close(client_fd);
                    continue;
                }
                if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                    std::cerr << "Failed to set socket to non-blocking." << std::endl;
                    close(client_fd);
                    continue;
                }
                // 注册client_fd到epoll实例
                event.data.fd = client_fd;
                //event.events = EPOLLIN | EPOLLET;
                event.events = EPOLLIN;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                    std::cerr << "Failed to add client_fd to epoll." << std::endl;
                    close(client_fd);
                    continue;
                }
                std::cerr << "new client[" << client_fd << "]" << std::endl;
            } else {
                // 处理客户端数据
                char buffer[1024];
                ssize_t count = read(events[i].data.fd, buffer, sizeof(buffer));
                if (count == -1) {
                    std::cerr << "Failed to read from socket." << std::endl;
                } else if (count == 0) {
                    // 客户端关闭了连接
                    close(events[i].data.fd);
                    std::cerr << "client[" << events[i].data.fd << "] closed!" << std::endl;
                } else {
                    MessageLogin* login = (MessageLogin*)buffer;
                    // 输出接收到的数据
                    //std::cout << "Received data: " << std::string(buffer, count) << "    ---->     dataLen : " << count << std::endl;
                    std::cout << "Received data: [" << login->username <<"] [" << login->password << "]    ---->    " << recvCount++ << std::endl;
                }
            }
        }
    }

    // 清理资源
    close(sfd);
    close(epoll_fd);
    return 0;
}