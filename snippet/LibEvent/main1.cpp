#include <iostream>

#include <event2/event.h>


void timerCb(evutil_socket_t fd, short events, void *arg)
{
    std::cout << "timer fired" << std::endl;
}


int main()
{
    // event_base_new()：初始化 event_base 结构，当于 epoll 红黑树的树根节点。成功返回 event_base 结构体指针，失败返回 NULL。
    event_base *base = event_base_new();

    /**
    event_new(): event_new 负责创建 event 结构指针, 同时指定对应的 base(epfd), 还有对应的文件描述符, 事件, 以及回调函数和回调函数的参数。
    参数说明：
        base: 对应的 event_base 根结点。
        fd: 要监听的文件描述符。
        events: 要监听的事件。
            #define  EV_TIMEOUT    0x01    //超时事件
            #define  EV_READ       0x02    //读事件
            #define  EV_WRITE      0x04    //写事件
            #define  EV_SIGNAL     0x08    //信号事件
            #define  EV_PERSIST    0x10    //周期性触发
            #define  EV_ET         0x20    //边缘触发, 如果底层模型支持设置则有效, 若不支持则无效。
            若要想设置持续的读事件则： EV_READ | EV_PERSIST
        cb: 回调函数, 原型如下：typedef void (*event_callback_fn)(evutil_socket_t fd, short events, void *arg)。注意: 回调函数的参数就对应于event_new 函数的 fd, event 和 arg。
        arg: 回调函数的参数。
    */
    // 创建一个定时事件。
    event *ev = event_new(
        base,
        -1,
        EV_TIMEOUT,
        timerCb,
        nullptr);

    timeval tv = {
        .tv_sec = 1,
        .tv_usec = 0,
    };

    // event_add()：将非未决态事件转为未决态, 相当于调用 epoll_ctl 函数（EPOLL_CTL_ADD），开始监听事件是否产生, 相当于 epoll 的上树操作。
    // ev: 调用 event_new 创建的事件。
    // timeout: 限时等待事件的产生(定时事件使用), 也可以设置为 NULL, 没有限时。
    event_add(ev, &tv);

    // event_base_dispatch(): 进入循环等待事件，也就是事件循环。
    // 由 event_base_new() 函数返回的指向 event_base 结构的指针调用该函数, 相当于标志位设置为 0 的 event_base_loop。程序将会一直运行, 直到没有需要检测的事件了, 或者被结束循环的 API 终止。
    /**
    还有另一个函数 int event_base_loop(struct event_base *base, int flags);
        base: 由 event_base_new 函数返回的指向 event_base 结构的指针。
        flags 的取值：
            #define EVLOOP_ONCE	0x01 只运行一次事件循环（一次循环可以处理多个事件），然后返回。如果事件没有被触发, 阻塞等待。
            #define EVLOOP_NONBLOCK	0x02 非阻塞方式检查一次事件，但是不等待。不管事件触发与否, 都会立即返回。
    */
    event_base_dispatch(base);

    // event_free()：释放由 event_new 申请的 event 节点。
    event_free(ev);

    // event_base_free()：释放 event_base 指针。
    event_base_free(base);
}
