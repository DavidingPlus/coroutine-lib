#include <gtest/gtest.h>

#include <atomic>
#include <vector>

#include "timer.h"

#include "config.h"


TEST(TimeWheelTest, EmptyTick)
{
    TimerManager mgr;

    EXPECT_NO_THROW(
        {
            mgr.tick();
        });
}

TEST(TimeWheelTest, OneTimer)
{
    TimerManager mgr(8);
    int count = 0;

    mgr.addTimer(3,
                 [&]
                 {
                     ++count;
                 });

    EXPECT_EQ(count, 0);

    // 执行槽 1,2 的任务。
    for (int i = 1; i <= 2; ++i)
    {
        mgr.tick();
        EXPECT_EQ(count, 0);
    }

    // 执行槽 3 的任务。
    mgr.tick();
    EXPECT_EQ(count, 1);
}

TEST(TimeWheelTest, MultipleTimers)
{
    TimerManager mgr(8);
    std::vector<int> vec;

    mgr.addTimer(1, [&]
                 { vec.push_back(1); });
    mgr.addTimer(3, [&]
                 { vec.push_back(3); });
    mgr.addTimer(5, [&]
                 { vec.push_back(5); });

    mgr.tick();
    ASSERT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], 1);

    mgr.tick();
    EXPECT_EQ(vec.size(), 1);

    mgr.tick();
    ASSERT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[1], 3);

    mgr.tick();
    EXPECT_EQ(vec.size(), 2);

    mgr.tick();
    ASSERT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[2], 5);
}

TEST(TimeWheelTest, SameSlotDifferentRound)
{
    TimerManager mgr(8);
    std::vector<int> vec;

    mgr.addTimer(1, [&]
                 { vec.push_back(1); });
    mgr.addTimer(9, [&]
                 { vec.push_back(9); });
    mgr.addTimer(17, [&]
                 { vec.push_back(17); });

    for (int i = 1; i <= 17; ++i) mgr.tick();

    ASSERT_EQ(vec.size(), 3);

    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 9);
    EXPECT_EQ(vec[2], 17);
}

TEST(TimeWheelTest, ZeroTimeout)
{
    TimerManager mgr(8);

    bool called = false;

    if (COROUTINE_CONFIG_DEBUG)
    {
        ASSERT_DEATH({ mgr.addTimer(0,
                                    [&]
                                    {
                                        called = true;
                                    }); },
                     "");
    }
}

TEST(TimeWheelTest, SameExpire)
{
    TimerManager mgr(8);
    int count = 0;

    for (int i = 0; i < 3; ++i)
    {
        mgr.addTimer(3, [&]
                     { ++count; });
    }

    for (int i = 1; i <= 3; ++i) mgr.tick();

    EXPECT_EQ(count, 3);
}

TEST(TimeWheelTest, CrossRound)
{
    TimerManager mgr(8);
    bool called = false;

    mgr.addTimer(15,
                 [&]
                 {
                     called = true;
                 });

    for (int i = 1; i <= 14; ++i) mgr.tick();

    EXPECT_FALSE(called);
    mgr.tick();
    EXPECT_TRUE(called);
}

TEST(TimeWheelTest, ManyTimers)
{
    TimerManager mgr(16);

    int count = 0;

    for (int i = 1; i <= 100; ++i)
    {
        mgr.addTimer(i,
                     [&]
                     {
                         ++count;
                     });
    }

    for (int i = 1; i <= 100; ++i) mgr.tick();

    EXPECT_EQ(count, 100);
}


int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);


    return RUN_ALL_TESTS();
}
