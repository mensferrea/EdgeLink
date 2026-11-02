#include "edgelink/network/epoll_event_loop.hpp"
#include <sys/eventfd.h>
#include <unistd.h>
#include <system_error>

namespace edgelink::network {

EpollEventLoop::EpollEventLoop() {
    epoll_fd_ = ::epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ < 0) {
        throw std::system_error(errno, std::generic_category(), "epoll_create1 failed");
    }

    wakeup_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wakeup_fd_ < 0) {
        ::close(epoll_fd_);
        throw std::system_error(errno, std::generic_category(), "eventfd failed");
    }

    add_fd(wakeup_fd_, EPOLLIN);
}

EpollEventLoop::~EpollEventLoop() {
    stop();
    if (wakeup_fd_ >= 0) {
        ::close(wakeup_fd_);
    }
    if (epoll_fd_ >= 0) {
        ::close(epoll_fd_);
    }
}

void EpollEventLoop::add_fd(int fd, uint32_t events, void* user_data) {
    epoll_event ev{};
    ev.events = events;
    ev.data.ptr = user_data;
    if (user_data == nullptr) {
        ev.data.fd = fd;
    }
    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
        throw std::system_error(errno, std::generic_category(), "epoll_ctl(ADD) failed");
    }
}

void EpollEventLoop::modify_fd(int fd, uint32_t events, void* user_data) {
    epoll_event ev{};
    ev.events = events;
    ev.data.ptr = user_data;
    if (user_data == nullptr) {
        ev.data.fd = fd;
    }
    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) < 0) {
        throw std::system_error(errno, std::generic_category(), "epoll_ctl(MOD) failed");
    }
}

void EpollEventLoop::remove_fd(int fd) {
    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        if (errno != ENOENT && errno != EBADF) {
            throw std::system_error(errno, std::generic_category(), "epoll_ctl(DEL) failed");
        }
    }
}

int EpollEventLoop::wait(std::span<epoll_event> event_buffer, int timeout_ms) {
    int nfds = ::epoll_wait(epoll_fd_, event_buffer.data(), static_cast<int>(event_buffer.size()), timeout_ms);
    if (nfds < 0) {
        if (errno == EINTR) {
            return 0;
        }
        throw std::system_error(errno, std::generic_category(), "epoll_wait failed");
    }
    return nfds;
}

void EpollEventLoop::wakeup() noexcept {
    if (wakeup_fd_ >= 0) {
        uint64_t val = 1;
        ssize_t w = ::write(wakeup_fd_, &val, sizeof(val));
        (void)w;
    }
}

void EpollEventLoop::stop() noexcept {
    running_.store(false, std::memory_order_release);
    wakeup();
}

bool EpollEventLoop::is_running() const noexcept {
    return running_.load(std::memory_order_acquire);
}

int EpollEventLoop::wakeup_fd() const noexcept {
    return wakeup_fd_;
}

}
