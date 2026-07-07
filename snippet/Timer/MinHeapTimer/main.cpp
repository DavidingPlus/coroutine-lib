#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <chrono>
#include <vector>

#include "timer.h"


static uint64_t getNowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}


TEST(TimerTest, BasicExpire)
{
    TimerManager tmr;
    std::atomic<bool> called = false;
    uint64_t now = getNowMs();

    tmr.add(
        now + 50,
        [&]()
        {
            called = true;
        });

    // 未到期，不应该执行。
    tmr.tick();
    EXPECT_FALSE(called);

    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    // 到期执行。
    tmr.tick();
    EXPECT_TRUE(called);
}

TEST(TimerTest, NotExpire)
{
    TimerManager tmr;
    bool called = false;
    uint64_t now = getNowMs();

    tmr.add(
        now + 1000,
        [&]()
        {
            called = true;
        });

    tmr.tick();
    EXPECT_FALSE(called);
}

TEST(TimerTest, ExecuteOrder)
{
    TimerManager tmr;
    std::vector<int> vec;
    uint64_t now = getNowMs();

    tmr.add(
        now + 100,
        [&]()
        {
            vec.push_back(1);
        });

    tmr.add(
        now + 10,
        [&]()
        {
            vec.push_back(2);
        });

    tmr.add(
        now + 50,
        [&]()
        {
            vec.push_back(3);
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(120));

    tmr.tick();

    ASSERT_EQ(vec.size(), 3);

    EXPECT_EQ(vec[0], 2);
    EXPECT_EQ(vec[1], 3);
    EXPECT_EQ(vec[2], 1);
}

TEST(TimerTest, SameExpireTime)
{
    TimerManager tmr;
    std::atomic<int> count = 0;
    uint64_t now = getNowMs();

    for (int i = 0; i < 10; ++i)
    {
        tmr.add(
            now + 20,
            [&]()
            {
                ++count;
            });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    tmr.tick();
    EXPECT_EQ(count, 10);
}

TEST(TimerTest, EmptyTick)
{
    TimerManager tmr;

    EXPECT_NO_FATAL_FAILURE(tmr.tick());
}

TEST(TimerTest, CancelTimer)
{
    TimerManager tmr;
    bool called = false;
    uint64_t now = getNowMs();

    auto id = tmr.add(
        now + 50,
        [&]()
        {
            called = true;
        });

    // 删除 Timer。
    tmr.cancel(id);

    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    tmr.tick();
    EXPECT_FALSE(called);
}

TEST(TimerTest, CancelMiddleTimer)
{
    TimerManager tmr;
    std::vector<int> vec;
    uint64_t now = getNowMs();

    auto id1 = tmr.add(
        now + 100,
        [&]()
        {
            vec.push_back(1);
        });

    auto id2 = tmr.add(
        now + 50,
        [&]()
        {
            vec.push_back(2);
        });

    auto id3 = tmr.add(
        now + 150,
        [&]()
        {
            vec.push_back(3);
        });

    // 删除中间节点。
    tmr.cancel(id2);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    tmr.tick();

    ASSERT_EQ(vec.size(), 2);

    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 3);
}

TEST(TimerTest, CancelInvalidId)
{
    TimerManager tmr;
    bool called = false;
    uint64_t now = getNowMs();

    tmr.add(
        now + 20,
        [&]()
        {
            called = true;
        });

    // 不存在的 ID。
    tmr.cancel(999999);

    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    tmr.tick();
    EXPECT_TRUE(called);
}

TEST(TimerTest, HeapAdjust)
{
    TimerManager tmr;
    std::atomic<int> count = 0;
    uint64_t now = getNowMs();
    std::vector<TimerManager::TimerId> ids;

    for (int i = 0; i < 100; ++i)
    {
        ids.push_back(
            tmr.add(
                now + i,
                [&]()
                {
                    ++count;
                }));
    }

    // 删除随机位置。
    for (size_t i = 0; i < ids.size(); i += 2) tmr.cancel(ids[i]);

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    tmr.tick();
    EXPECT_EQ(count, 50);
}


int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);


    return RUN_ALL_TESTS();
}
