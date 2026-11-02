#include <gtest/gtest.h>
#include "edgelink/sync/thread_safe_queue.hpp"
#include <thread>
#include <vector>
#include <numeric>

TEST(ThreadSafeQueueTest, BasicPushPop) {
    edgelink::sync::ThreadSafeQueue<int> queue(10);
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);

    EXPECT_TRUE(queue.push(100));
    EXPECT_TRUE(queue.push(200));
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 2);

    int val = 0;
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 100);

    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 200);

    EXPECT_TRUE(queue.empty());
}

TEST(ThreadSafeQueueTest, DropOldestPolicy) {
    edgelink::sync::ThreadSafeQueue<int> queue(3);

    EXPECT_TRUE(queue.try_push_drop_oldest(1));
    EXPECT_TRUE(queue.try_push_drop_oldest(2));
    EXPECT_TRUE(queue.try_push_drop_oldest(3));
    EXPECT_EQ(queue.dropped_count(), 0);

    EXPECT_TRUE(queue.try_push_drop_oldest(4));
    EXPECT_EQ(queue.dropped_count(), 1);
    EXPECT_EQ(queue.size(), 3);

    int val = 0;
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 2);
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 3);
    EXPECT_TRUE(queue.pop(val));
    EXPECT_EQ(val, 4);
}

TEST(ThreadSafeQueueTest, MultiProducerMultiConsumer) {
    edgelink::sync::ThreadSafeQueue<int> queue(1000);
    const int producers = 4;
    const int consumers = 4;
    const int items_per_producer = 2500;
    const int total_items = producers * items_per_producer;

    std::vector<std::thread> prod_threads;
    for (int p = 0; p < producers; ++p) {
        prod_threads.emplace_back([&queue, p, items_per_producer]() {
            for (int i = 0; i < items_per_producer; ++i) {
                queue.push(p * items_per_producer + i);
            }
        });
    }

    std::atomic<int> consumed_count{0};
    std::vector<std::thread> cons_threads;
    for (int c = 0; c < consumers; ++c) {
        cons_threads.emplace_back([&queue, &consumed_count]() {
            int val = 0;
            while (queue.pop(val)) {
                ++consumed_count;
            }
        });
    }

    for (auto& t : prod_threads) {
        t.join();
    }

    while (consumed_count.load() < total_items) {
        std::this_thread::yield();
    }

    queue.stop();

    for (auto& t : cons_threads) {
        t.join();
    }

    EXPECT_EQ(consumed_count.load(), total_items);
    EXPECT_EQ(queue.pushed_count(), total_items);
    EXPECT_EQ(queue.popped_count(), total_items);
}

TEST(ThreadSafeQueueTest, StopUnblocksWaitingThreads) {
    edgelink::sync::ThreadSafeQueue<int> queue(5);
    std::atomic<bool> thread_exited{false};

    std::thread consumer([&queue, &thread_exited]() {
        int val = 0;
        bool res = queue.pop(val);
        EXPECT_FALSE(res);
        thread_exited = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(thread_exited);

    queue.stop();
    consumer.join();

    EXPECT_TRUE(thread_exited);
    EXPECT_TRUE(queue.is_stopped());
}
