#include <gtest/gtest.h>
#include "edgelink/sync/thread_pool.hpp"
#include <vector>
#include <numeric>

TEST(ThreadPoolTest, ExecuteTasksWithFutures) {
    edgelink::sync::ThreadPool pool(4);
    EXPECT_EQ(pool.size(), 4);

    std::vector<std::future<int>> futures;
    for (int i = 0; i < 20; ++i) {
        futures.push_back(pool.submit([i]() {
            return i * i;
        }));
    }

    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(futures[i].get(), i * i);
    }
}

TEST(ThreadPoolTest, ConcurrentStressExecution) {
    edgelink::sync::ThreadPool pool(8);
    std::atomic<int> counter{0};
    const int total_tasks = 1000;

    std::vector<std::future<void>> futures;
    futures.reserve(total_tasks);

    for (int i = 0; i < total_tasks; ++i) {
        futures.push_back(pool.submit([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        }));
    }

    for (auto& f : futures) {
        f.get();
    }

    EXPECT_EQ(counter.load(), total_tasks);
}
