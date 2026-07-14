#include <gtest/gtest.h>

#include "hook.h"

#include "fdmanager.h"
#include "iomanager.h"

#include <atomic>
#include <thread>
#include <chrono>
#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>


TEST(HookTest, HookEnableDefault)
{
    setHookEnable(false);

    EXPECT_FALSE(isHookEnable());

    setHookEnable(false);
}

TEST(HookTest, HookEnableOpen)
{
    setHookEnable(true);

    EXPECT_TRUE(isHookEnable());

    setHookEnable(false);
}

TEST(HookTest, HookEnableClose)
{
    setHookEnable(true);
    setHookEnable(false);

    EXPECT_FALSE(isHookEnable());

    setHookEnable(false);
}

TEST(SocketHookTest, TCPRegister)
{
    setHookEnable(true);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(fd, 0);

    auto ctx = FdMgr::GetInstance()->get(fd);
    ASSERT_NE(ctx, nullptr);

    EXPECT_TRUE(ctx->isSocket());
    EXPECT_FALSE(ctx->isClosed());

    EXPECT_EQ(close(fd), 0);

    setHookEnable(false);
}

TEST(SocketHookTest, UDPRegister)
{
    setHookEnable(true);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT_GE(fd, 0);

    auto ctx = FdMgr::GetInstance()->get(fd);
    ASSERT_NE(ctx, nullptr);

    EXPECT_TRUE(ctx->isSocket());

    EXPECT_EQ(close(fd), 0);

    setHookEnable(false);
}

TEST(SocketHookTest, CreateFailed)
{
    setHookEnable(true);

    int fd = socket(-1, SOCK_STREAM, 0);
    EXPECT_EQ(fd, -1);

    setHookEnable(false);
}

TEST(CloseHookTest, RemoveFdCtx)
{
    setHookEnable(true);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(fd, 0);

    ASSERT_NE(FdMgr::GetInstance()->get(fd), nullptr);
    EXPECT_EQ(close(fd), 0);
    EXPECT_EQ(FdMgr::GetInstance()->get(fd), nullptr);

    setHookEnable(false);
}

TEST(CloseHookTest, ClosePipe)
{
    setHookEnable(true);

    int fd[2];

    ASSERT_EQ(pipe(fd), 0);
    EXPECT_EQ(close(fd[0]), 0);
    EXPECT_EQ(close(fd[1]), 0);

    setHookEnable(false);
}

TEST(CloseHookTest, DoubleClose)
{
    setHookEnable(true);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(fd, 0);

    EXPECT_EQ(close(fd), 0);
    EXPECT_EQ(close(fd), -1);

    setHookEnable(false);
}

TEST(SocketTimeoutTest, ReceiveTimeout)
{
    setHookEnable(true);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(fd, 0);

    timeval tv = {
        .tv_sec = 3,
        .tv_usec = 500000,
    };

    ASSERT_EQ(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)), 0);

    auto ctx = FdMgr::GetInstance()->get(fd);
    ASSERT_NE(ctx, nullptr);

    EXPECT_EQ(ctx->getTimeout(SO_RCVTIMEO), 3500);

    EXPECT_EQ(close(fd), 0);

    setHookEnable(false);
}

TEST(SocketTimeoutTest, SendTimeout)
{
    setHookEnable(true);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(fd, 0);

    timeval tv = {
        .tv_sec = 1,
        .tv_usec = 0,
    };

    ASSERT_EQ(setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)), 0);

    auto ctx = FdMgr::GetInstance()->get(fd);
    ASSERT_NE(ctx, nullptr);

    EXPECT_EQ(ctx->getTimeout(SO_SNDTIMEO), 1000);

    EXPECT_EQ(close(fd), 0);

    setHookEnable(false);
}

TEST(SocketTimeoutTest, NormalOption)
{
    setHookEnable(true);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(fd, 0);

    int val = 1;
    EXPECT_EQ(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)), 0);

    EXPECT_EQ(close(fd), 0);

    setHookEnable(false);
}

TEST(IOHookTest, PipeReadWrite)
{
    setHookEnable(true);

    int fd[2];
    ASSERT_EQ(pipe(fd), 0);

    ASSERT_EQ(write(fd[1], "hello", 5), 5);

    char buf[16] = {};

    ASSERT_EQ(read(fd[0], buf, 16), 5);
    EXPECT_STREQ(buf, "hello");

    EXPECT_EQ(close(fd[0]), 0);
    EXPECT_EQ(close(fd[1]), 0);

    setHookEnable(false);
}

TEST(IOHookTest, PipeMultiReadWrite)
{
    setHookEnable(true);

    int fd[2];
    ASSERT_EQ(pipe(fd), 0);

    write(fd[1], "abc", 3);
    write(fd[1], "def", 3);

    char buf[8] = {};

    EXPECT_EQ(read(fd[0], buf, 6), 6);
    EXPECT_STREQ(buf, "abcdef");

    EXPECT_EQ(close(fd[0]), 0);
    EXPECT_EQ(close(fd[1]), 0);

    setHookEnable(false);
}

TEST(IOHookTest, InvalidRead)
{
    setHookEnable(true);

    char buf[8];

    EXPECT_EQ(read(-1, buf, sizeof(buf)), -1);
    EXPECT_EQ(errno, EBADF);

    setHookEnable(false);
}

TEST(HookIOTest, ReadvWritevBasic)
{
    setHookEnable(true);

    int fd[2];
    ASSERT_EQ(pipe(fd), 0);

    char msg1[] = "hello ";
    char msg2[] = "world";

    iovec wiov[2];

    wiov[0].iov_base = msg1;
    wiov[0].iov_len = strlen(msg1);

    wiov[1].iov_base = msg2;
    wiov[1].iov_len = strlen(msg2);

    ASSERT_EQ(writev(fd[1], wiov, 2), 11);

    char buf1[7] = {};
    char buf2[6] = {};

    iovec riov[2];

    riov[0].iov_base = buf1;
    riov[0].iov_len = 6;

    riov[1].iov_base = buf2;
    riov[1].iov_len = 5;

    ASSERT_EQ(readv(fd[0], riov, 2), 11);

    EXPECT_STREQ(buf1, "hello ");
    EXPECT_STREQ(buf2, "world");

    EXPECT_EQ(close(fd[0]), 0);
    EXPECT_EQ(close(fd[1]), 0);

    setHookEnable(false);
}

TEST(HookIOTest, ReadvWritevManyBuffers)
{
    setHookEnable(true);

    int fd[2];
    ASSERT_EQ(pipe(fd), 0);

    std::vector<std::string> data{"a", "bb", "ccc", "dddd"};
    std::vector<iovec> wiov;

    for (auto &s : data)
    {
        iovec v;
        v.iov_base = (void *)s.data();
        v.iov_len = s.size();

        wiov.push_back(v);
    }

    size_t total = 10;

    ASSERT_EQ(writev(fd[1], wiov.data(), wiov.size()), total);

    char buffer[32] = {};

    EXPECT_EQ(read(fd[0], buffer, sizeof(buffer)), total);
    EXPECT_STREQ(buffer, "abbcccdddd");

    EXPECT_EQ(close(fd[0]), 0);
    EXPECT_EQ(close(fd[1]), 0);

    setHookEnable(false);
}

// TEST(HookIOTest, ReadvWaitEvent)
// {
//     setHookEnable(true);

//     int fd[2];
//     ASSERT_EQ(pipe(fd), 0);

//     IOManager iom(2, false);
//     std::atomic<bool> finished = false;

//     iom.scheduleLock([&]()
//                      {
//                          char a[8] = {};
//                          char b[8] = {};

//                          iovec iov[2];

//                          iov[0].iov_base = a;
//                          iov[0].iov_len = 7;

//                          iov[1].iov_base = b;
//                          iov[1].iov_len = 7;

//                          auto n = readv(fd[0], iov, 2);
//                          EXPECT_EQ(n, 7);

//                          EXPECT_STREQ(a, "hello");

//                          finished = true; //
//                      });

//     std::this_thread::sleep_for(std::chrono::milliseconds(100));

//     EXPECT_FALSE(finished);

//     write(fd[1], "hello", 5);
//     write(fd[1], "!!", 2);

//     std::this_thread::sleep_for(std::chrono::milliseconds(200));
//     EXPECT_TRUE(finished);

//     EXPECT_EQ(close(fd[0]), 0);
//     EXPECT_EQ(close(fd[1]), 0);
// }

// TEST(AcceptHookTest, RegisterAcceptedFd)
// {
//     setHookEnable(true);

//     int listenFd = socket(AF_INET, SOCK_STREAM, 0);
//     ASSERT_GE(listenFd, 0);

//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

//     bind(listenFd, (sockaddr *)&addr, sizeof(addr));

//     listen(listenFd, 5);

//     socklen_t len = sizeof(addr);
//     getsockname(listenFd, (sockaddr *)&addr, &len);

//     std::thread t([&]()
//                   {
//                       int fd = socket(AF_INET, SOCK_STREAM, 0);

//                       connect(fd, (sockaddr *)&addr, sizeof(addr));

//                       close(fd); //
//                   });

//     int clientFd = accept(listenFd, nullptr, nullptr);
//     ASSERT_GE(clientFd, 0);

//     auto ctx = FdMgr::GetInstance()->get(clientFd);
//     ASSERT_NE(ctx, nullptr);
//     EXPECT_TRUE(ctx->isSocket());

//     close(clientFd);
//     close(listenFd);

//     t.join();
// }

// TEST(SleepHookTest, FiberSchedule)
// {
//     setHookEnable(true);

//     IOManager iom(2, false);
//     std::vector<int> order;

//     iom.scheduleLock([&]()
//                      {
//                          order.push_back(1);

//                          sleep(1);

//                          order.push_back(3); //
//                      });

//     iom.scheduleLock([&]()
//                      {
//                          order.push_back(2); //
//                      });

//     std::this_thread::sleep_for(std::chrono::milliseconds(200));

//     ASSERT_EQ(order.size(), 2);
//     EXPECT_EQ(order[0], 1);
//     EXPECT_EQ(order[1], 2);

//     std::this_thread::sleep_for(std::chrono::milliseconds(1000));

//     ASSERT_EQ(order.size(), 3);
//     EXPECT_EQ(order[2], 3);
// }
