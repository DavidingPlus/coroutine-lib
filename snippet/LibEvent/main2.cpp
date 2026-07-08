// 同时监听 IO 事件和定时事件。一个 event_base 可以统一管理多个不同类型的事件，不同事件源不需要创建额外线程，而是在同一个事件循环中等待。
#include <iostream>

#include <event2/event.h>

#include <unistd.h>


void readCb(evutil_socket_t fd, short events, void *arg)
{
    char buffer[128];
    int n = read(fd, buffer, sizeof(buffer));

    if (n > 0) std::cout << "recv: " << std::string(buffer, n) << std::endl;
}

void timerCb(evutil_socket_t fd, short events, void *arg)
{
    std::cout << "timer tick" << std::endl;
}

int main()
{
    event_base *base = event_base_new();

    // 注册 stdin 可读事件。
    event *stdinEv = event_new(
        base,
        STDIN_FILENO,
        EV_READ | EV_PERSIST, // 监听可读事件并且事件触发后不自动删除。
        readCb,
        nullptr);

    // 注册 timer 事件。
    event *timerEv = event_new(
        base,
        -1,
        EV_TIMEOUT | EV_PERSIST,
        timerCb,
        nullptr);

    event_add(stdinEv, nullptr);

    timeval tv = {
        .tv_sec = 1,
        .tv_usec = 0,
    };
    event_add(timerEv, &tv);

    event_base_dispatch(base);

    event_free(timerEv);
    event_free(stdinEv);

    event_base_free(base);
}
