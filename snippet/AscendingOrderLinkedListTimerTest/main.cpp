#include <gtest/gtest.h>

#include "timer.h"


TEST(TimerManagerTest, EmptyTick)
{
    TimerManager mgr;

    EXPECT_NO_THROW(
        {
            mgr.tick();
        });
}


int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);


    return RUN_ALL_TESTS();
}
