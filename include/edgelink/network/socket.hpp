#pragma once

#include <string>
#include <cstdint>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace edgelink::network {

class Socket {
public:
    Socket() noexcept;
    explicit Socket(int fd) noexcept;
    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    static Socket create_tcp();
    static Socket create_udp();
    static Socket create_raw_icmp();

    static std::string resolve_host(const std::string& host);

    void bind(uint16_t port, const std::string& ip = "0.0.0.0");
    void listen(int backlog = SOMAXCONN);
    [[nodiscard]] Socket accept(std::string& client_ip, uint16_t& client_port);
    void connect(const std::string& host_or_ip, uint16_t port);

    void set_nonblocking(bool nonblocking = true);
    void set_reuse_address(bool reuse = true);
    void set_reuse_port(bool reuse = true);
    void set_receive_timeout(uint32_t timeout_ms);

    [[nodiscard]] int native_handle() const noexcept;
    [[nodiscard]] bool is_valid() const noexcept;
    void close() noexcept;

private:
    int fd_{-1};
};

}
