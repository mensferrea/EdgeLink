#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <optional>
#include <chrono>

namespace edgelink::sync {

template <typename T>
class ThreadSafeQueue {
public:
    explicit ThreadSafeQueue(size_t capacity = 10000)
        : capacity_(capacity) {}

    ~ThreadSafeQueue() {
        stop();
    }

    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    bool push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_push_.wait(lock, [this]() {
            return stopped_ || queue_.size() < capacity_;
        });

        if (stopped_) {
            return false;
        }

        queue_.push(std::move(item));
        ++pushed_count_;
        lock.unlock();
        cv_pop_.notify_one();
        return true;
    }

    bool try_push_drop_oldest(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (stopped_) {
            return false;
        }

        if (queue_.size() >= capacity_) {
            queue_.pop();
            ++dropped_count_;
        }

        queue_.push(std::move(item));
        ++pushed_count_;
        lock.unlock();
        cv_pop_.notify_one();
        return true;
    }

    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_pop_.wait(lock, [this]() {
            return stopped_ || !queue_.empty();
        });

        if (queue_.empty()) {
            return false;
        }

        item = std::move(queue_.front());
        queue_.pop();
        ++popped_count_;
        lock.unlock();
        cv_push_.notify_one();
        return true;
    }

    std::optional<T> pop_with_timeout(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!cv_pop_.wait_for(lock, timeout, [this]() { return stopped_ || !queue_.empty(); })) {
            return std::nullopt;
        }

        if (queue_.empty()) {
            return std::nullopt;
        }

        T item = std::move(queue_.front());
        queue_.pop();
        ++popped_count_;
        lock.unlock();
        cv_push_.notify_one();
        return item;
    }

    void stop() noexcept {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        cv_pop_.notify_all();
        cv_push_.notify_all();
    }

    [[nodiscard]] bool is_stopped() const noexcept {
        return stopped_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] size_t size() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    [[nodiscard]] size_t capacity() const noexcept {
        return capacity_;
    }

    [[nodiscard]] uint64_t dropped_count() const noexcept {
        return dropped_count_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] uint64_t pushed_count() const noexcept {
        return pushed_count_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] uint64_t popped_count() const noexcept {
        return popped_count_.load(std::memory_order_relaxed);
    }

private:
    const size_t capacity_;
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_pop_;
    std::condition_variable cv_push_;
    std::atomic<bool> stopped_{false};
    std::atomic<uint64_t> dropped_count_{0};
    std::atomic<uint64_t> pushed_count_{0};
    std::atomic<uint64_t> popped_count_{0};
};

}
