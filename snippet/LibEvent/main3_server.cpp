#include <iostream>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

#include <event2/event.h>


struct ClientContext
{
    // 服务端监听该客户端 fd 的可读事件。当客户端发送数据后，数据进入服务端 TCP 接收缓冲区，libevent 通知该事件，然后执行 read 操作。
    event *readEv = nullptr;

    // 服务端监听该客户端 fd 的可写事件。当 TCP 发送缓冲区存在空间，并且 outputBuffer 中存在待发送数据时，libevent 通知该事件，然后执行 write 操作。
    event *writeEv = nullptr;

    // 用户态发送缓冲区。保存服务端已经产生但尚未发送完成的数据。由于 TCP 内核发送缓冲区大小有限，当 write 无法一次发送全部数据时，剩余数据暂存在这里，等待 socket 再次触发 EV_WRITE 后继续发送。这样避免因为一次 write 失败而丢失业务数据。
    std::string outputBuffer;
};


void setNonblock(int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

// 服务端写数据回调函数。
void writeCb(evutil_socket_t fd, short events, void *arg)
{
    ClientContext *ctx = static_cast<ClientContext *>(arg);

    if (ctx->outputBuffer.empty())
    {
        // 没有业务数据需要发送。虽然 socket 仍然可能处于可写状态，但是没有必要继续监听 EV_WRITE，否则会因为 socket 长期可写导致事件循环不断触发。
        event_del(ctx->writeEv);
        return;
    }

    int n = write(fd, ctx->outputBuffer.data(), ctx->outputBuffer.size());
    if (n > 0)
    {
        // 删除已经发送的数据。
        ctx->outputBuffer.erase(0, n);

        // 数据全部发送完成。
        if (ctx->outputBuffer.empty())
        {
            event_del(ctx->writeEv);
        }
        // 当内核 TCP 发送缓冲区空间不足时，write() 很可能没有一次发送完全部数据。由于未设置 EV_PERSIST，写完就会自动删除事件，但是还有数据没写完，因此重新添加事件继续监听。
        else
        {
            event_add(ctx->writeEv, nullptr);
        }
    }
    else
    {
        if (EAGAIN == errno || EWOULDBLOCK == errno) return;

        perror("write");

        event_del(ctx->readEv);
        event_del(ctx->writeEv);

        event_free(ctx->readEv);
        event_free(ctx->writeEv);

        close(fd);

        delete ctx;
    }
}

// 服务端读数据回调函数。
void readCb(evutil_socket_t fd, short events, void *arg)
{
    ClientContext *ctx = static_cast<ClientContext *>(arg);
    char buffer[1024];

    int n = read(fd, buffer, sizeof(buffer));
    if (n > 0)
    {
        std::cout << "client recv: " << std::string(buffer, n) << std::endl;

        // Echo：将收到的数据原样写回客户端。
        // 当前直接调用 write 完成 Echo，仅适用于数据量较小的场景。如果客户端接收速度较慢，内核 TCP 发送缓冲区可能被写满，write 会返回 EAGAIN，这样未写入的数据会丢失，而对服务端来讲不能出现这样的问题。因此需要增加额外的 outputBuffer 保存待发送的数据，并注册 EV_WRITE 事件，在 socket 可写时继续发送，而不是直接丢弃。这样做的目的是避免不断轮询 write，而是将控制权交给 epoll，在 socket 可写时再继续发送。
        // 与之对应，客户端发送数据时也是一样的道理。如果服务端读取速度跟不上，服务端内核的 TCP 接收缓冲区会被填满，TCP 会通过流量控制（滑动窗口）通知客户端减慢发送速度。此时客户端调用 send/write 同样可能阻塞或返回 EAGAIN，客户端也需要自己决定如何处理未发送的数据。因此，这部分发送失败的处理责任属于发送方，而服务端这里之所以要维护 outputBuffer，是因为这些数据属于服务端的业务数据，不能因为一次 write 失败而直接丢弃。发送失败的数据由发送方负责保存。

        // 判断当前是否需要添加写事件。只有当 outputBuffer 原本为空时，才需要添加 EV_WRITE。因为如果 outputBuffer 已经存在数据，说明之前已经开启了写事件，当前 socket 仍然处于等待可写状态，无需重复添加。
        // 当 write 过程中出现 EAGAIN 时，表示 TCP 发送缓冲区暂时已满，此时未发送的数据仍然保存在 outputBuffer 中，而 writeEv 继续监听 socket 可写事件。等发送缓冲区再次有空间时，libevent 会触发写事件继续发送。
        // 因此需要保证一个状态：
        // outputBuffer 非空时，writeEv 必须处于监听状态；
        // outputBuffer 为空时，表示没有待发送数据，需要关闭 writeEv，避免无意义监听。
        bool needWrite = ctx->outputBuffer.empty();

        ctx->outputBuffer.append(buffer, n);

        // 服务端读取到客户端数据后，需要进行 Echo 回写。只有从无待发送数据变为有待发送数据时，才开启写事件。
        if (needWrite) event_add(ctx->writeEv, nullptr);
    }
    else if (0 == n)
    {
        // 客户端关闭连接。
        std::cout << "client closed" << std::endl;

        // 删除事件。
        event_del(ctx->readEv);
        event_del(ctx->writeEv);

        close(fd);

        // 释放事件。
        event_free(ctx->readEv);
        event_free(ctx->writeEv);

        delete ctx;
    }
    else
    {
        // 当前没有数据。不阻塞等待。继续交给 event loop 处理。
        if (EAGAIN == errno || EWOULDBLOCK == errno) return;

        perror("read");

        event_del(ctx->readEv);
        event_del(ctx->writeEv);

        close(fd);

        event_free(ctx->readEv);
        event_free(ctx->writeEv);

        delete ctx;
    }
}

// 服务端接受连接回调函数。
void acceptCb(evutil_socket_t fd, short events, void *arg)
{
    event_base *base = static_cast<event_base *>(arg);

    while (true)
    {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);

        int clientFd = accept(fd, (sockaddr *)&client_addr, &len);
        if (clientFd < 0)
        {
            // 非阻塞 accept：没有更多连接，退出循环。
            if (EAGAIN == errno || EWOULDBLOCK == errno) break;

            perror("accept");
            return;
        }

        // 设置非阻塞。
        setNonblock(clientFd);

        std::cout << "new client connected, fd=" << clientFd << std::endl;

        // 创建客户端上下文。
        ClientContext *ctx = new ClientContext;

        // 创建可读事件。监听服务端对应客户端 fd 的可读状态。当客户端发送数据后，数据进入服务端 TCP 接收缓冲区，当接收缓冲区中存在数据时，socket 变为可读状态，libevent 通知该事件，然后执行 readCb。如果 TCP 接收缓冲区为空，则 socket 不处于可读状态，epoll 不会持续返回该事件，因此可以长期监听。这里使用 EV_PERSIST，使该事件在触发一次后仍然保持监听，用于处理客户端后续发送的数据。这是典型的状态驱动。
        ctx->readEv = event_new(
            base,
            clientFd,
            EV_READ | EV_PERSIST,
            readCb,
            ctx);

        // 创建可写事件。这里只创建，不立即 event_add。因为 TCP socket 通常一直处于可写状态。注意：可写事件表示内核 TCP 发送缓冲区存在空闲空间，而不是表示应用层有数据需要发送。即使 TCP 发送缓冲区为空，socket 依然属于可写状态，如果持续监听 EV_WRITE，epoll 会不断返回可写事件，导致 write 回调被频繁触发，造成 CPU 空转。因此写事件采用按需开启的方式：只有 outputBuffer 中存在待发送数据时才添加 EV_WRITE，数据发送完成后立即删除写事件。这是典型的需求驱动。
        ctx->writeEv = event_new(
            base,
            clientFd,
            EV_WRITE,
            writeCb,
            ctx);

        event_add(ctx->readEv, nullptr);
    }
}


int main()
{
    // 1. 创建 socket 函数。
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0)
    {
        perror("socket");
        return -1;
    }

    // 2. 设置端口复用。
    int reuse = 1;
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // 3. bind 端口。
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8888);

    if (bind(listenFd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        return -1;
    }

    // 4. listen 开始监听端口。
    if (listen(listenFd, SOMAXCONN) < 0)
    {
        perror("listen");
        return -1;
    }

    // 设置非阻塞。
    setNonblock(listenFd);

    std::cout << "server listen on port 8888" << std::endl;

    // 5. 注册 listenFd 事件。
    event_base *base = event_base_new();

    // 监控 listenFd，当它变成可读状态时，调用 acceptCb。
    event *listenEv = event_new(
        base,
        listenFd,
        EV_READ | EV_PERSIST,
        acceptCb,
        base);

    event_add(listenEv, nullptr);

    event_base_dispatch(base);

    event_free(listenEv);

    event_base_free(base);

    close(listenFd);
}
