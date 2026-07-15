#ifndef SOCKET_H
#define SOCKET_H

// RAII 封装 socket 文件描述符:对象销毁时自动 close
class Socket {
public:
    explicit Socket(int fd);
    ~Socket();

    Socket(const Socket&) = delete;             // 禁拷贝
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other) noexcept;            // 允许移动

    int get() const;
    bool valid() const;

private:
    int fd_;
};

#endif  // SOCKET_H