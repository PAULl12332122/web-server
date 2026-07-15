#include "Socket.h"
#include <iostream>
#include <unistd.h>     // close

Socket::Socket(int fd) : fd_(fd) {}

Socket::~Socket() {
    if (fd_ >= 0) {
        close(fd_);
        std::cout << "[Socket] fd " << fd_ << " closed automatically\n";
    }
}

Socket::Socket(Socket&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

int Socket::get() const { return fd_; }

bool Socket::valid() const { return fd_ >= 0; }