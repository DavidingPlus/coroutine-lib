#include <gtest/gtest.h>

#include "fiber.h"

#include "config.h"


// 主协程（Main Fiber）由 Fiber::GetThis() 首次调用时创建，并由线程局部变量。thread_local std::shared_ptr<Fiber> t_threadFiber 持有。由于 thread_local 对象的生命周期贯穿整个线程，因此即使单元测试已经结束，主协程也不会立即析构。当测试程序退出、线程结束时，t_threadFiber 才会析构，shared_ptr 引用计数归零，最终触发主协程的析构函数，因此会在所有测试输出完成后看到如下输出，这反而是正常现象。
// [----------] Global test environment tear-down
// [==========] 31 tests from 3 test suites ran. (570 ms total)
// [  PASSED  ] 31 tests.
// ~Fiber(): id = 1
TEST(FiberTest, GetThis)
{
    auto fiber = Fiber::GetThis();

    ASSERT_NE(fiber, nullptr);
    EXPECT_EQ(fiber->getState(), Fiber::RUNNING);
}

TEST(FiberTest, CreateFiber)
{
    Fiber::GetThis();

    auto fiber = std::make_shared<Fiber>(
        [] {},
        0,
        false);

    ASSERT_NE(fiber, nullptr);
    EXPECT_EQ(fiber->getState(), Fiber::READY);
}

TEST(FiberTest, Resume)
{
    Fiber::GetThis();

    bool executed = false;

    auto fiber = std::make_shared<Fiber>(
        [&]
        {
            executed = true;
        },
        0,
        false);

    EXPECT_FALSE(executed);

    fiber->resume();

    EXPECT_TRUE(executed);
    EXPECT_EQ(fiber->getState(), Fiber::TERMINATE);
}

TEST(FiberTest, Reset)
{
    Fiber::GetThis();

    int value = 0;

    auto fiber = std::make_shared<Fiber>(
        [&]
        {
            value = 1;
        },
        0,
        false);

    EXPECT_EQ(value, 0);

    fiber->resume();
    EXPECT_EQ(value, 1);

    fiber->reset([&]
                 {
                     value = 2; //
                 });
    EXPECT_EQ(fiber->getState(), Fiber::READY);

    fiber->resume();
    EXPECT_EQ(value, 2);
}

TEST(FiberTest, GetFiberId)
{
    auto fiber = Fiber::GetThis();

    EXPECT_EQ(Fiber::GetFiberId(), fiber->getId());
}

TEST(FiberTest, FiberIdUnique)
{
    Fiber::GetThis();

    auto f1 = std::make_shared<Fiber>([] {}, 0, false);
    auto f2 = std::make_shared<Fiber>([] {}, 0, false);

    EXPECT_NE(f1->getId(), f2->getId());
}

TEST(FiberTest, GetThisInsideFiber)
{
    Fiber::GetThis();

    Fiber *current = nullptr;

    auto fiber = std::make_shared<Fiber>(
        [&]
        {
            current = Fiber::GetThis().get();
        },
        0,
        false);

    fiber->resume();

    EXPECT_EQ(current, fiber.get());
}

TEST(FiberTest, MultipleResume)
{
    Fiber::GetThis();

    int count = 0; // 同一个线程中的协程是依次执行的，因此不存在竞争需要加锁的问题，也就不需要使用原子变量。

    std::vector<std::shared_ptr<Fiber>> fibers;

    for (int i = 0; i < 5; ++i)
    {
        fibers.push_back(std::make_shared<Fiber>(
            [&]
            {
                ++count;
            },
            0,
            false));
    }

    for (auto &fiber : fibers) fiber->resume();

    EXPECT_EQ(count, 5);
}

TEST(FiberTest, ResumeAfterTerminate)
{
    Fiber::GetThis();

    auto fiber = std::make_shared<Fiber>(
        [] {},
        0,
        false);

    // 已经结束的协程不能再次 resume()，需要 reset() 才能再次运行。
    fiber->resume();

    // Release 模式下，assert(xxx) 宏会被定义为 NDEBUG，该部分的代码都会被优化掉，因此下面这行代码只在 Debug 下测试。
    if (COROUTINE_CONFIG_DEBUG) ASSERT_DEATH({ fiber->resume(); }, "");
}
