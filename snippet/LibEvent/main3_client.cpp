#include <gtest/gtest.h>

#include <iostream>
#include <string>
#include <thread>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>


class EchoClient
{

public:

    EchoClient()
    {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        EXPECT_GT(fd, 0);

        // 设置接收超时，避免测试永久阻塞。
        timeval tv{};
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    ~EchoClient()
    {
        if (fd >= 0) close(fd);
    }

    bool connectServer()
    {
        sockaddr_in addr{};

        addr.sin_family = AF_INET;
        addr.sin_port = htons(8888);

        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);


        return 0 == connect(fd, (sockaddr *)&addr, sizeof(addr));
    }

    // 保证发送完整数据。
    bool sendAll(const std::string &data)
    {
        size_t total = 0;

        while (total < data.size())
        {
            int n = send(fd, data.data() + total, data.size() - total, 0);
            if (n <= 0) return false;

            total += n;
        }


        return true;
    }

    // TCP 是字节流，需要循环接收。
    std::string recvData(size_t size)
    {
        std::string res;
        res.reserve(size);

        while (res.size() < size)
        {
            char buffer[4096];

            int n = recv(fd, buffer, sizeof(buffer), 0);
            if (n <= 0) break;

            res.append(buffer, n);
        }


        return res;
    }


private:

    int fd = -1;
};


// 测试普通 Echo。
TEST(EchoServerTest, BasicEcho)
{
    EchoClient client;
    ASSERT_TRUE(client.connectServer());

    std::string msg = "hello libevent";

    ASSERT_TRUE(client.sendAll(msg));

    auto res = client.recvData(msg.size());
    EXPECT_EQ(res, msg);
}

// 测试大数据 Echo。
TEST(EchoServerTest, LargeMessage)
{
    EchoClient client;
    ASSERT_TRUE(client.connectServer());

    // 64 KB 数据。
    std::string msg(64 * 1024, 'A');

    ASSERT_TRUE(client.sendAll(msg));

    auto res = client.recvData(msg.size());

    EXPECT_EQ(res.size(), msg.size());
    EXPECT_EQ(res, msg);
}

// 测试多个客户端。
TEST(EchoServerTest, MultipleClient)
{
    for (int i = 0; i < 10; ++i)
    {
        EchoClient client;
        ASSERT_TRUE(client.connectServer());

        std::string msg = "client-" + std::to_string(i);

        ASSERT_TRUE(client.sendAll(msg));

        auto res = client.recvData(msg.size());
        EXPECT_EQ(res, msg);
    }
}

// 空消息测试。
TEST(EchoServerTest, ClientCloseImmediately)
{
    EchoClient client;
    ASSERT_TRUE(client.connectServer());

    // 不发送数据，直接析构关闭。
}

// 多客户端并发测试。
TEST(EchoServerTest, ConcurrentClients)
{
    std::vector<std::thread> threads;

    for (int i = 0; i < 50; ++i)
    {
        threads.emplace_back([i]()
                             {
                                 EchoClient client;
                                 ASSERT_TRUE(client.connectServer());

                                 std::string msg = "hello-" + std::to_string(i);

                                 ASSERT_TRUE(client.sendAll(msg));

                                 auto res = client.recvData(msg.size());
                                 EXPECT_EQ(res, msg); //
                             });
    }

    for (auto &t : threads) t.join();
}

// 大包小包测试。
TEST(EchoServerTest, ManySmallMessages)
{
    EchoClient client;

    ASSERT_TRUE(client.connectServer());

    for (int i = 0; i < 1000; ++i)
    {
        std::string msg = "msg-" + std::to_string(i);

        ASSERT_TRUE(client.sendAll(msg));

        auto res = client.recvData(msg.size());
        EXPECT_EQ(res, msg);
    }
}

// 客户端异常断开。
TEST(EchoServerTest, ClientAbort)
{
    EchoClient client;

    ASSERT_TRUE(client.connectServer());

    std::string msg(1024 * 1024, 'C');

    client.sendAll(msg);

    // 析构立即关闭连接。
}

// 超大数据测试。
TEST(EchoServerTest, HugeMessage)
{
    EchoClient client;

    ASSERT_TRUE(client.connectServer());

    // 100 MB。
    std::string msg(100 * 1024 * 1024, 'B');

    ASSERT_TRUE(client.sendAll(msg));

    auto res = client.recvData(msg.size());
    EXPECT_EQ(res.size(), msg.size());
}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);


    return RUN_ALL_TESTS();
}
