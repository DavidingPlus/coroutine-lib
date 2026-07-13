#include <gtest/gtest.h>

#include "iomanager.h"

#include <atomic>

#include <unistd.h>


TEST(IOManagerTest, ReadEvent)
{
    IOManager iom(2, true); // 成功。
    // IOManager iom(2, false); // 成功
    // IOManager iom(1, true);  // 失败。这个是因为主线程参与工作调度的情况下，目前的设计需要 IOManager 析构时调用 stop() 才会调度主线程，因此肯定是失败的。

    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);

    std::atomic<bool> called = false;

    std::cout << "scheduler= " << Scheduler::GetThis() << std::endl;

    char buffer[16] = {0};
    auto res = iom.addEvent(
        pipefd[0],
        IOManager::Event::READ,
        [&]()
        {
            called = true;

            read(pipefd[0], buffer, sizeof(buffer));
        });

    ASSERT_EQ(res, 0);

    // 触发读事件。
    write(pipefd[1], "hello", 5);

    // 等待调度。
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_TRUE(called);
    EXPECT_STREQ(buffer, "hello");

    close(pipefd[0]);
    close(pipefd[1]);
}

TEST(IOManagerTest, WriteEvent)
{
    IOManager iom(1, false);

    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);

    std::atomic<bool> called = false;

    iom.addEvent(
        pipefd[1],
        IOManager::Event::WRITE,
        [&]()
        {
            called = true;

            write(pipefd[1], "hello", 5);
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_TRUE(called);

    char buffer[16] = {0};
    read(pipefd[0], buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, "hello");

    close(pipefd[0]);
    close(pipefd[1]);
}

TEST(IOManagerTest, ReadWriteEvent)
{
    IOManager iom(1, false);

    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);

    std::atomic<int> count = 0;

    iom.addEvent(
        pipefd[0],
        IOManager::Event::READ,
        [&]()
        {
            ++count;
        });

    iom.addEvent(
        pipefd[1],
        IOManager::Event::WRITE,
        [&]()
        {
            ++count;
        });

    write(pipefd[1], "x", 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_EQ(count, 2);


    close(pipefd[0]);
    close(pipefd[1]);
}
