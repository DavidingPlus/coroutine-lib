#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <atomic>

#include "timer.h"


TEST(TimerManagerTest, EmptyTick)
{
    TimerManager mgr;

    EXPECT_NO_THROW(
        {
            mgr.tick();
        });
}

TEST(TimerManagerTest, OneTimer)
{
    TimerManager mgr;
    std::atomic_int count = 0;
    uint64_t now =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();

    mgr.addTimer(
        new Timer(
            now + 100,
            [&]()
            {
                ++count;
            }));

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    EXPECT_EQ(count, 0);
    mgr.tick();
    EXPECT_EQ(count, 1);
}

TEST(TimerManagerTest, MultipleTimer)
{
    TimerManager mgr;
    std::vector<int> res;
    uint64_t now =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();

    mgr.addTimer(new Timer(now + 300,
                           [&]
                           {
                               res.push_back(3);
                           }));

    mgr.addTimer(new Timer(now + 100,
                           [&]
                           {
                               res.push_back(1);
                           }));

    mgr.addTimer(new Timer(now + 200,
                           [&]
                           {
                               res.push_back(2);
                           }));

    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    mgr.tick();

    ASSERT_EQ(res.size(), 3);

    EXPECT_EQ(res[0], 1);
    EXPECT_EQ(res[1], 2);
    EXPECT_EQ(res[2], 3);
}

TEST(TimerManagerTest, NotExpire)
{
    TimerManager mgr;
    bool called = false;
    uint64_t now =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();

    mgr.addTimer(new Timer(now + 500,
                           [&]
                           {
                               called = true;
                           }));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    mgr.tick();
    EXPECT_FALSE(called);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    mgr.tick();
    EXPECT_TRUE(called);
}

TEST(TimerManagerTest, AdjustTimer)
{
    TimerManager mgr;
    bool called = false;
    uint64_t now =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();

    auto timer = new Timer(now + 100,
                           [&]
                           {
                               called = true;
                           });

    mgr.addTimer(timer);

    // 延长到 500ms。
    mgr.adjustTimer(timer, now + 500);

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    mgr.tick();
    EXPECT_FALSE(called);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    mgr.tick();
    EXPECT_TRUE(called);
}

TEST(TimerManagerTest, DeleteTimer)
{
    TimerManager mgr;
    bool called = false;
    uint64_t now =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();

    auto timer = new Timer(now + 100,
                           [&]
                           {
                               called = true;
                           });

    mgr.addTimer(timer);

    mgr.delTimer(timer);
    delete timer;

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    mgr.tick();
    EXPECT_FALSE(called);
}

TEST(TimerManagerTest, TickManyTimes)
{
    TimerManager mgr;
    int count = 0;
    uint64_t now =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();

    mgr.addTimer(new Timer(now + 100, [&]
                           { ++count; }));
    mgr.addTimer(new Timer(now + 200, [&]
                           { ++count; }));
    mgr.addTimer(new Timer(now + 300, [&]
                           { ++count; }));

    std::this_thread::sleep_for(std::chrono::milliseconds(110));
    mgr.tick();
    EXPECT_EQ(count, 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(110));
    mgr.tick();
    EXPECT_EQ(count, 2);

    std::this_thread::sleep_for(std::chrono::milliseconds(110));
    mgr.tick();
    EXPECT_EQ(count, 3);
}

#if COROUTINE_CONFIG_DEBUG

TEST(TimerManagerTest, InsertOrder)
{
    TimerManager mgr;

    mgr.addTimer(new Timer(300, [] {}));
    mgr.addTimer(new Timer(100, [] {}));
    mgr.addTimer(new Timer(200, [] {}));

    mgr.printTimers();
}

#endif


int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);


    return RUN_ALL_TESTS();
}
