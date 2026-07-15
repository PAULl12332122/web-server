#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>          // epoll_create1 / epoll_ctl / epoll_wait
#include "Socket.h"

const int MAX_EVENTS = 1024;    // epoll_wait 一次最多返回多少个就绪事件

int main(){
    // 1)建监听 socket
    Socket listener(socket(AF_INET, SOCK_STREAM, 0));
    if (!listener.valid()){
        std::cerr << "socket failed\n";
        return 1;
    }

    int opt = 1;
    setsockopt(listener.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);
    if (bind(listener.get(), (sockaddr*)&addr, sizeof(addr)) < 0){
        std::cerr << "bind fail\n";
        return 1;
    }
    if (listen(listener.get(), SOMAXCONN) < 0){
        std::cerr << "listen failed\n";
        return 1;
    }

    // 2) 创建 epoll 实例, 拿到 epoll fd
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0){
        std::cerr << "epoll_create1 failed\n";
        return 1;
    }

    // 3) 把【监听 socket】登记进epoll, 关心它的【可读】（有新连接排队）
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listener.get();
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener.get(), &ev);

    epoll_event events[MAX_EVENTS];    // epoll_wait 用来装返回的就绪事件

    // 4) 事件循环 —— 这就是 Reactor 的心脏
    while (true){
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);   // -1 = 一直阻塞到有事件

        for (int i = 0; i < n; ++i){
            int fd = events[i].data.fd;

            if (fd == listener.get()){
                // ——监听 fd 就绪: 有新连接, accept 它并登记进 epoll ——
                int client_fd = accept(listener.get(), nullptr, nullptr);
                if (client_fd < 0) continue;

                epoll_event cev{};
                cev.events = EPOLLIN;
                cev.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &cev);
                std::cout << "New client: fd " << client_fd << "\n";
                
            } else{
                // —— 某个客户 fd 就绪: 读数据, 回声 ——
                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));
                ssize_t cnt = read(fd, buffer, sizeof(buffer) - 1);

                if (cnt <= 0) {
                    // 断开：先从 epoll 名单摘掉，再 close
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                    std::cout << "Client fd " << fd << " disconnected\n";
                } else {
                    std::cout << "Received from fd " << fd << ": " << buffer;
                    write(fd, buffer, cnt);   // 回声
                }
            }
        }
    }
    
    close(epoll_fd);
    return 0;
}