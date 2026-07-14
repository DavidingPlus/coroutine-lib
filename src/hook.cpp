#include "hook.h"

#include "fdmanager.h"
#include "iomanager.h"

#include "config.h"

#include <iostream>

#include <dlfcn.h>


// 宏 HOOK_FUN(XX) 是一个宏展开机制，通过将 XX 依次应用于宏定义中的每一个函数名称来生成一系列代码。这种方式可以有效减少重复代码，提高代码的可读性和维护性。
#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)       \
    XX(nanosleep)    \
    XX(socket)       \
    XX(connect)      \
    XX(accept)       \
    XX(read)         \
    XX(readv)        \
    XX(recv)         \
    XX(recvfrom)     \
    XX(recvmsg)      \
    XX(write)        \
    XX(writev)       \
    XX(send)         \
    XX(sendto)       \
    XX(sendmsg)      \
    XX(close)        \
    XX(fcntl)        \
    XX(ioctl)        \
    XX(getsockopt)   \
    XX(setsockopt)


// 使用线程局部变量，每个线程都会判断一下是否启用了钩子。
// 表示当前线程是否启用了钩子功能。初始值为 false，即钩子功能默认关闭。
static thread_local bool tHookEnable = false;


bool isHookEnable()
{
    return tHookEnable;
}

void setHookEnable(bool flag)
{
    tHookEnable = flag;
}

void hookInit()
{
    // 通过一个静态变量来确保 hookInit() 只初始化一次，防止重复初始化。
    static bool isInited = false;
    if (isInited) return;

    isInited = true;

// 由于 hook 库通过 LD_PRELOAD 机制被动态链接器优先加载，例如当程序调用 sleep() 时，动态链接器会优先解析到当前 hook 库中的 sleep 实现，而不是 libc.so 中原始的 sleep 函数。
// 但是在 hook 函数内部，我们仍然需要调用真正的 libc 实现，否则直接调用 sleep() 会再次进入自己的 hook 函数，导致无限递归。
// 因此需要使用 dlsym() 获取原始函数地址。RTLD_NEXT 表示从当前动态库之后继续搜索符号，跳过当前 hook 库，从后续加载的共享库中查找真实实现。
// 例如：
//
//      调用 sleep()
//            |
//            v
//      libhook.so:sleep()       <-- 当前 hook 函数
//            |
//            | dlsym(RTLD_NEXT, "sleep")
//            v
//      libc.so:sleep()          <-- 原始系统调用
//
// 最终 sleep_f 保存的就是 libc 中真实 sleep 函数的地址，hook 函数可以通过 sleep_f 间接调用原始实现。

// 使用 X-Macro 技巧批量初始化 hook 函数对应的原始函数指针。
//
// HOOK_FUN(XX) 会遍历所有需要 hook 的函数，并将函数名传递给 XX 宏。
// 例如：
//
// XX(sleep)
//
// 会展开为：
//
// sleep_f = (sleep_fun)dlsym(RTLD_NEXT, "sleep");
//
// 其中：
// - name##_f：## 用于拼接变量名，例如 sleep -> sleep_f。
// - name##_fun：拼接函数指针类型，例如 sleep -> sleep_fun。
// - #name：将函数名转换为字符串，例如 sleep -> "sleep"。
//
// 最终 HOOK_FUN(XX) 会展开成所有 hook 函数的 dlsym 初始化代码，避免为每个函数重复手写初始化逻辑。
#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX)
#undef XX
}


struct HookIniter
{
    // 钩子函数。初始化 hook，让原始调用绑定到宏展开的函数指针中。
    HookIniter() { hookInit(); }
};

// 定义了一个静态的 HookIniter 实例。由于静态变量的初始化发生在 main() 函数之前，所以 hookInit() 会在程序开始时被调用，从而初始化钩子函数。
static HookIniter sHookIniter;


// 用于跟踪定时器的状态。它有一个 cancelled 成员变量，通常用于表示定时器是否已经被取消。
struct TimerInfo
{
    // 表示定时器是否已经被取消。
    int cancelled = 0;
};


/**
 * @brief 将阻塞式 IO 操作转换为协程异步 IO。
 *
 * doIo() 是对阻塞式 IO 操作 read/write/recv/send/connect 等可能阻塞的系统调用的统一封装。它根据当前文件描述符状态决定是否直接调用原始系统调用，或者将可能阻塞的 IO 操作转换为协程异步等待。当 IO 暂时无法完成时，doIo 会将当前协程挂起，并通过 IOManager 等待对应事件发生；如果设置了超时时间，则结合定时器机制处理超时情况。最终，doIo 将传统阻塞 IO 转换为基于事件驱动的协程 IO，避免阻塞线程，提高并发处理能力。
 *
 * 整体执行流程：
 *
 *  1. 调用原始系统 IO 函数（libc 函数）
 *
 *                    +----------------+
 *                    | fun(fd, args)  |
 *                    +----------------+
 *                            |
 *                            v
 *                  +-------------------+
 *                  | 返回值 >= 0 ?     |
 *                  +-------------------+
 *                     |             |
 *                    是             否
 *                     |             |
 *                     v             v
 *              返回 IO 结果     errno == EINTR ?
 *                                   |
 *                         +---------+---------+
 *                         |                   |
 *                        是                   否
 *                         |                   |
 *                         v                   v
 *                      retry           errno == EAGAIN ?
 *                                             |
 *                                  +----------+----------+
 *                                  |                     |
 *                                 否                     是
 *                                  |                     |
 *                                  v                     v
 *                             返回错误          当前 IO 暂不可用
 *
 *
 *  2. EAGAIN 处理流程：
 *
 *             errno == EAGAIN
 *                    |
 *                    v
 *        +-------------------------+
 *        | 获取 IOManager          |
 *        +-------------------------+
 *                    |
 *                    v
 *        是否设置 IO 超时时间？
 *                    |
 *          +---------+---------+
 *          |                   |
 *         是                   否
 *          |                   |
 *          v                   |
 *     添加条件定时器              |
 *          |                   |
 *          +---------+---------+
 *                    |
 *                    v
 *        IOManager::addEvent(fd,event)
 *                    |
 *          +---------+---------+
 *          |                   |
 *         失败                 成功
 *          |                   |
 *          v                   v
 *     取消 timer          当前 Fiber yield()
 *     返回错误                   |
 *                              |
 *                              v
 *                 +------------+------------+
 *                 |                         |
 *             IO事件触发                 Timer超时
 *                 |                         |
 *                 v                         v
 *          IOManager恢复Fiber        timer callback
 *                 |                         |
 *                 |                  设置 ETIMEDOUT
 *                 |                  cancelEvent()
 *                 |                         |
 *                 +------------+------------+
 *                              |
 *                              v
 *                    Fiber 从 yield 恢复
 *                              |
 *                              v
 *                       取消超时 timer
 *                              |
 *                 +------------+------------+
 *                 |                         |
 *             超时结束                  IO事件完成
 *                 |                         |
 *                 v                         v
 *          errno=ETIMEDOUT              goto retry
 *          return -1                       |
 *                                           v
 *                                重新调用 fun(fd,args...)
 *
 */
/**
 * @param fd 需要进行 IO 操作的文件描述符。
 * @param fun 原始系统调用函数指针，例如 read_f、write_f，用于在满足条件时调用真实 libc 函数。
 * @param hookFunName 当前 hook 函数名称，仅用于调试输出和错误定位。
 * @param event 需要监听的 IO 事件类型，例如 IOManager::READ 或 IOManager::WRITE。当 IO 暂时不可用时，IOManager 根据该事件等待 fd 状态变化。
 * @param timeoutSo 超时选项对应的 socket 参数，例如 SO_RCVTIMEO 或 SO_SNDTIMEO，用于从 FdCtx 中获取对应 IO 超时时间。
 * @param args 传递给原始系统调用的参数。使用完美转发保持参数原始类型，支持不同 IO 函数的参数形式。
 */
template <typename OriginFun, typename... Args>
static ssize_t doIo(int fd, OriginFun fun, const char *hookFunName, uint32_t event, int timeoutSo, Args &&...args)
{
    // 如果全局钩子功能未启用，则直接调用原始的 I/O 函数。
    if (tHookEnable) return fun(fd, std::forward<Args>(args)...);

    // 获取与文件描述符 fd 相关联的上下文 ctx。
    std::shared_ptr<FdCtx> ctx = FdMgr::GetInstance()->get(fd);
    // 如果上下文不存在，则直接调用原始的 I/O 函数。
    if (!ctx) return fun(fd, std::forward<Args>(args)...);
    // 如果文件描述符已经关闭，设置 errno 为 EBADF 并返回 -1。
    if (ctx->isClosed())
    {
        // Bad file number，表示文件描述符无效或已经关闭。
        errno = EBADF;
        return -1;
    }
    // 如果文件描述符不是一个 socket 或者用户设置了非阻塞模式，则直接调用原始的 I/O 操作函数。
    if (!ctx->isSocket() || ctx->getUserNonblock()) return fun(fd, std::forward<Args>(args)...);

    // 获取超时设置并初始化 TimerInfo 结构体，用于后续的超时管理和取消操作。
    uint64_t timeout = ctx->getTimeout(timeoutSo);
    std::shared_ptr<TimerInfo> tinfo(new TimerInfo);

retry:
    // 执行原始 I/O 函数。如果正确执行，则会按照原始 I/O 函数正常返回退出函数。doIo() hook 的是 I/O 操作失败时需要额外做的事情。
    ssize_t n = fun(fd, std::forward<Args>(args)...);

    // 如果 I/O 系统调用失败，并且失败原因是被信号打断（EINTR），那么重新执行一次系统调用，直到不是因为 EINTR 的原因返回。
    while (-1 == n && EINTR == errno) n = fun(fd, std::forward<Args>(args)...);

    // 如果 I/O 操作因为资源暂时不可用（EAGAIN）而失败，函数会添加一个事件监听器来等待资源可用。同时，如果有超时设置，还会启动一个条件计时器来取消事件。
    if (-1 == n && EAGAIN == errno)
    {
        IOManager *iom = IOManager::GetThis();
        std::shared_ptr<Timer> timer;
        std::weak_ptr<TimerInfo> winfo(tinfo);

        // 1. 如果执行的 read 等函数在 FdManager 管理的 FdCtx 中 fd 设置了超时时间，就会走到这里。添加 addconditionTimer() 事件。
        if (static_cast<uint64_t>(-1) != timeout) // 超时时间默认是 -1。
        {
            timer = iom->addConditionTimer(
                timeout,
                [winfo, fd, iom, event]()
                {
                    auto t = winfo.lock();
                    // 如果 TimerInfo 对象已被释放（!t），或者操作已被取消（t->cancelled 非 0），则直接返回。
                    if (!t || t->cancelled) return;

                    // 超时时间到达，记录这次 IO 等待结束的原因是“超时”。
                    t->cancelled = ETIMEDOUT;
                    // 取消该文件描述符上的事件，并立即触发一次事件。
                    iom->cancelEvent(fd, static_cast<IOManager::Event>(event));
                },
                winfo);
        }

        // 2. 将 fd（文件描述符）和 event（要监听的事件，如读或写事件）添加到 IOManager 中进行管理。IOManager 会监听这个文件描述符上的事件，当事件触发时，它会调度相应的协程来处理这个事件。
        int res = iom->addEvent(fd, static_cast<IOManager::Event>(event));
        // 如果 res 为 -1，说明 addEvent 失败。取消之前设置的定时器，避免误触发。
        if (-1 == res)
        {
            if (COROUTINE_CONFIG_DEBUG) std::cout << hookFunName << " addEvent(" << fd << ", " << event << ") failed";

            if (timer) timer->cancel();


            return -1;
        }
        // 如果 addEvent 成功（res 为 0），当前协程会调用 yield() 函数，将自己挂起，等待事件的触发。
        else
        {
            Fiber::GetThis()->yield();

            // 3. 当协程被恢复时（例如，事件触发后），它会继续执行 yield() 之后的代码。如果之前设置了定时器（timer 不为 nullptr），则需取消该定时器。因为当前 Fiber 已经因为 I/O 事件或超时事件恢复执行，说明本次 I/O 等待已经结束。如果之前创建了超时定时器，需要将其取消，避免 I/O 已经完成后定时器仍然触发，干扰后续的 I/O 操作或重复处理事件。
            if (timer) timer->cancel();

            // 检查当前 Fiber 是否因为 I/O 超时而被唤醒。如果前面设置的条件定时器触发，会将 tinfo->cancelled 标记为 ETIMEDOUT，并通过 cancelEvent() 唤醒 Fiber，已经执行过 I/O 事件了，此时直接返回超时错误，不再重新尝试 I/O。
            if (ETIMEDOUT == tinfo->cancelled)
            {
                errno = tinfo->cancelled;
                return -1;
            }
            // 如果没有超时，则 retry 正常执行原始 I/O 函数，然后退出函数。
            else
            {
                goto retry;
            }
        }
    }


    return n;
}


extern "C"
{

// 使用 X-Macro 技巧批量定义所有 hook 函数对应的原始函数指针，并初始化为空。
// 例如：
//
// XX(sleep)
//
// 会展开为：
//
// sleep_fun sleep_f = nullptr;
//
// 最终 HOOK_FUN(XX) 会展开成：
//
// sleep_fun sleep_f = nullptr;
// usleep_fun usleep_f = nullptr;
// socket_fun socket_f = nullptr;
// connect_fun connect_f = nullptr;
// ...
//
// 后续在 hook 初始化阶段，再通过 dlsym(RTLD_NEXT, "...") 为这些函数指针赋值，使其指向 libc 中对应的原始系统调用实现。
#define XX(name) name##_fun name##_f = nullptr;
    HOOK_FUN(XX)
#undef XX


    // 实现了一个协程版本的 sleep，通过钩子机制拦截 sleep 的调用，并将其改为使用协程来实现非阻塞的休眠。
    unsigned int sleep(unsigned int seconds)
    {
        // 如果钩子未启用，则调用原始的系统调用。
        if (!tHookEnable) return sleep_f(seconds);

        // 获取当前正在执行的协程（Fiber），并将其保存到 fiber 变量中。
        std::shared_ptr<Fiber> fiber = Fiber::GetThis();
        // 获取当前线程的调度器对象（IOManager）指针，用于添加定时器任务。
        IOManager *iom = IOManager::GetThis();

        iom->addTimer(seconds * 1000, [fiber, iom]()
                      { iom->scheduleLock(fiber); });
        // 挂起当前协程的执行，将控制权交还给调度器。
        fiber->yield();


        return 0;
    }

    int usleep(useconds_t usec)
    {
        if (!tHookEnable) return usleep_f(usec);

        std::shared_ptr<Fiber> fiber = Fiber::GetThis();
        IOManager *iom = IOManager::GetThis();

        // useconds_t 一个无符号整数类型，通常用于表示微秒数。在这个函数中，usec 表示延时的微秒数，将其转换为毫秒数 (usec/1000) 后用于定时器。
        iom->addTimer(usec / 1000, [fiber, iom]()
                      { iom->scheduleLock(fiber); });
        fiber->yield();


        return 0;
    }

    int nanosleep(const struct timespec *req, struct timespec *rem)
    {
        if (!tHookEnable) return nanosleep_f(req, rem);

        // 将 tv_sec 转换为毫秒，并将 tv_nsec 转换为毫秒，然后两者相加得到总的超时毫秒数。所以从这里看出实现的也是一个毫秒级的操作。
        int timeoutMs = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;

        std::shared_ptr<Fiber> fiber = Fiber::GetThis();
        IOManager *iom = IOManager::GetThis();

        iom->addTimer(timeoutMs, [fiber, iom]()
                      { iom->scheduleLock(fiber); });
        fiber->yield();


        return 0;
    }


    ssize_t read(int fd, void *buf, size_t count)
    {
        return doIo(fd, read_f, "read", static_cast<uint32_t>(IOManager::Event::READ), SO_RCVTIMEO, buf, count);
    }

    ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
    {
        return doIo(fd, readv_f, "readv", static_cast<uint32_t>(IOManager::Event::READ), SO_RCVTIMEO, iov, iovcnt);
    }

    ssize_t recv(int sockfd, void *buf, size_t len, int flags)
    {
        return doIo(sockfd, recv_f, "recv", static_cast<uint32_t>(IOManager::Event::READ), SO_RCVTIMEO, buf, len, flags);
    }

    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
    {
        return doIo(sockfd, recvfrom_f, "recvfrom", static_cast<uint32_t>(IOManager::Event::READ), SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
    }

    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
    {
        return doIo(sockfd, recvmsg_f, "recvmsg", static_cast<uint32_t>(IOManager::Event::READ), SO_RCVTIMEO, msg, flags);
    }

    ssize_t write(int fd, const void *buf, size_t count)
    {
        return doIo(fd, write_f, "write", static_cast<uint32_t>(IOManager::Event::WRITE), SO_SNDTIMEO, buf, count);
    }

    ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
    {
        return doIo(fd, writev_f, "writev", static_cast<uint32_t>(IOManager::Event::WRITE), SO_SNDTIMEO, iov, iovcnt);
    }

    ssize_t send(int sockfd, const void *buf, size_t len, int flags)
    {
        return doIo(sockfd, send_f, "send", static_cast<uint32_t>(IOManager::Event::WRITE), SO_SNDTIMEO, buf, len, flags);
    }

    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
    {
        return doIo(sockfd, sendto_f, "sendto", static_cast<uint32_t>(IOManager::Event::WRITE), SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
    }

    ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
    {
        return doIo(sockfd, sendmsg_f, "sendmsg", static_cast<uint32_t>(IOManager::Event::WRITE), SO_SNDTIMEO, msg, flags);
    }
}
