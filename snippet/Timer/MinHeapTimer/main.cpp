#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <chrono>
#include <vector>

#include "timer.h"


TEST(TimerTest, BasicExpire)
{
    TimerManager tmr;
    std::atomic<bool> called = false;
    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

    tmr.add(
        Timer(
            now + 50,
            [&]()
            {
                called = true;
            }));

    tmr.tick();
    EXPECT_FALSE(called);

    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    tmr.tick();
    EXPECT_TRUE(called);
}

// 测试：未到期任务不会执行。
TEST(TimerTest, NotExpire)
{
    TimerManager tmr;
    bool called = false;
    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

    tmr.add(
        Timer(
            now + 1000,
            [&]()
            {
                called = true;
            }));

    tmr.tick();
    EXPECT_FALSE(called);
}

TEST(TimerTest, ExecuteOrder)
{
    TimerManager tmr;
    std::vector<int> vec;
    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

    tmr.add(
        Timer(
            now + 100,
            [&]()
            {
                vec.push_back(1);
            }));

    tmr.add(
        Timer(
            now + 10,
            [&]()
            {
                vec.push_back(2);
            }));

    tmr.add(
        Timer(
            now + 50,
            [&]()
            {
                vec.push_back(3);
            }));


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
    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

    for (int i = 0; i < 10; ++i)
    {
        tmr.add(
            Timer(
                now + 20,
                [&]()
                {
                    ++count;
                }));
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


int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);


    return RUN_ALL_TESTS();
}
