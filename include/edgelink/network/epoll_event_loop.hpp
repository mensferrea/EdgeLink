#pragma once

#include <sys/epoll.h>
#include <vector>
#include <atomic>
#include <span>

namespace edgelink::network {

class EpollEventLoop {
public:
    EpollEventLoop();
    ~EpollEventLoop();

    EpollEventLoop(const EpollEventLoop&) = delete;
    EpollEventLoop& operator=(const EpollEventLoop&) = delete;

    void add_fd(int fd, uint32_t events, void* user_data = nullptr);
    void modify_fd(int fd, uint32_t events, void* user_data = nullptr);
    void remove_fd(int fd);

    int wait(std::span<epoll_event> event_buffer, int timeout_ms = -1);
    void wakeup() noexcept;
    void stop() noexcept;
    [[nodiscard]] bool is_running() const noexcept;
    [[nodiscard]] int wakeup_fd() const noexcept;

private:
    int epoll_fd_{-1};
    int wakeup_fd_{-1};
    std::atomic<bool> running_{true};
};

}
