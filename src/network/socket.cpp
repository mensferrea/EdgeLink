#include "edgelink/network/socket.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <system_error>
#include <utility>

namespace edgelink::network {

Socket::Socket() noexcept : fd_(-1) {}

Socket::Socket(int fd) noexcept : fd_(fd) {}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        close();
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

Socket Socket::create_tcp() {
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        throw std::system_error(errno, std::generic_category(), "socket(TCP) failed");
    }
    return Socket(fd);
}

Socket Socket::create_udp() {
    int fd = ::socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        throw std::system_error(errno, std::generic_category(), "socket(UDP) failed");
    }
    return Socket(fd);
}

Socket Socket::create_raw_icmp() {
    int fd = ::socket(AF_INET, SOCK_RAW | SOCK_CLOEXEC, IPPROTO_ICMP);
    if (fd < 0) {
        throw std::system_error(errno, std::generic_category(), "socket(RAW_ICMP) failed");
    }
    return Socket(fd);
}

std::string Socket::resolve_host(const std::string& host) {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* res = nullptr;
    int err = ::getaddrinfo(host.c_str(), nullptr, &hints, &res);
    if (err != 0 || res == nullptr) {
        return host;
    }

    char ip_buf[INET_ADDRSTRLEN];
    auto* sin = reinterpret_cast<sockaddr_in*>(res->ai_addr);
    ::inet_ntop(AF_INET, &sin->sin_addr, ip_buf, sizeof(ip_buf));
    std::string ip_str = ip_buf;
    ::freeaddrinfo(res);
    return ip_str;
}

void Socket::bind(uint16_t port, const std::string& ip) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        throw std::system_error(errno, std::generic_category(), "inet_pton failed");
    }

    if (::bind(fd_, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::system_error(errno, std::generic_category(), "bind failed");
    }
}

void Socket::listen(int backlog) {
    if (::listen(fd_, backlog) < 0) {
        throw std::system_error(errno, std::generic_category(), "listen failed");
    }
}

Socket Socket::accept(std::string& client_ip, uint16_t& client_port) {
    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = ::accept4(fd_, reinterpret_cast<sockaddr*>(&client_addr), &addr_len, SOCK_CLOEXEC);
    if (client_fd < 0) {
        throw std::system_error(errno, std::generic_category(), "accept4 failed");
    }

    char ip_buf[INET_ADDRSTRLEN];
    ::inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, sizeof(ip_buf));
    client_ip = ip_buf;
    client_port = ntohs(client_addr.sin_port);

    return Socket(client_fd);
}

void Socket::connect(const std::string& host_or_ip, uint16_t port) {
    std::string ip = resolve_host(host_or_ip);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        throw std::system_error(errno, std::generic_category(), "inet_pton failed: " + ip);
    }

    if (::connect(fd_, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::system_error(errno, std::generic_category(), "connect failed");
    }
}

void Socket::set_nonblocking(bool nonblocking) {
    int flags = ::fcntl(fd_, F_GETFL, 0);
    if (flags < 0) {
        throw std::system_error(errno, std::generic_category(), "fcntl(F_GETFL) failed");
    }
    if (nonblocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    if (::fcntl(fd_, F_SETFL, flags) < 0) {
        throw std::system_error(errno, std::generic_category(), "fcntl(F_SETFL) failed");
    }
}

void Socket::set_reuse_address(bool reuse) {
    int opt = reuse ? 1 : 0;
    if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::system_error(errno, std::generic_category(), "setsockopt(SO_REUSEADDR) failed");
    }
}

void Socket::set_reuse_port(bool reuse) {
    int opt = reuse ? 1 : 0;
    if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        throw std::system_error(errno, std::generic_category(), "setsockopt(SO_REUSEPORT) failed");
    }
}

void Socket::set_receive_timeout(uint32_t timeout_ms) {
    timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if (::setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        throw std::system_error(errno, std::generic_category(), "setsockopt(SO_RCVTIMEO) failed");
    }
}

int Socket::native_handle() const noexcept {
    return fd_;
}

bool Socket::is_valid() const noexcept {
    return fd_ >= 0;
}

void Socket::close() noexcept {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

}
