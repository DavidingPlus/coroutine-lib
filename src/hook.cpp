#include "hook.h"

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
    int cancelled = 0;
};
