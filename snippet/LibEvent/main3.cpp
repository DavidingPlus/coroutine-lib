#include <iostream>

#include <event2/event.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>


void setNonblock(int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

void clientReadCb(evutil_socket_t fd, short events, void *arg)
{
    char buffer[1024];

    int n = read(fd, buffer, sizeof(buffer));

    if (n > 0)
    {
        std::cout << "client recv: " << std::string(buffer, n) << std::endl;

        // Echo：将收到的数据原样写回客户端。
        if (write(fd, buffer, n) < 0)
        {
            // 写缓冲区满了，暂时不能写。后面需要注册 EV_WRITE。
            if (EAGAIN == errno) return;

            perror("write");
            return;
        }
    }
    else if (0 == n)
    {
        // 客户端关闭连接。
        std::cout << "client closed" << std::endl;

        close(fd);
    }
    else
    {
        // 当前没有数据。不阻塞等待。继续交给 event loop 处理。
        if (EAGAIN == errno || EWOULDBLOCK == errno) return;

        perror("read");
        close(fd);
    }
}

void acceptCb(evutil_socket_t fd, short events, void *arg)
{
    sockaddr_in client_addr{};
    socklen_t len = sizeof(client_addr);

    int clientFd = accept(fd, (sockaddr *)&client_addr, &len);
    if (clientFd < 0)
    {
        perror("accept");
        return;
    }

    // 设置非阻塞。
    setNonblock(clientFd);

    std::cout << "new client connected, fd=" << clientFd << std::endl;

    event_base *base = static_cast<event_base *>(arg);

    event *client_event = event_new(
        base,
        clientFd,
        EV_READ | EV_PERSIST,
        clientReadCb,
        nullptr);

    event_add(client_event, nullptr);
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
