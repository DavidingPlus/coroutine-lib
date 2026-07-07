#include <iostream>

#include <ucontext.h>
#include <unistd.h>


int main(int argc, const char *argv[])
{
    ucontext_t context;

    // 程序在输出第一个 "Hello world" 后并没有退出程序，而是持续不断的输出 "Hello world"。其实是程序通过 getcontext 先保存了一个上下文,然后输出 "Hello world",在通过 setcontext 恢复到 getcontext 的地方，重新执行代码，所以导致程序不断的输出 "Hello world"。

    // 初始化 ucp 结构体，将当前的上下文保存到 ucp 中。
    getcontext(&context);

    std::cout << "Hello world" << std::endl;
    sleep(1);

    // 设置当前的上下文为 ucp，setcontext 的上下文 ucp 应该通过 getcontext 或者 makecontext 取得，如果调用成功则不返回。如果上下文是通过调用 getcontext() 取得,程序会继续执行这个调用。如果上下文是通过调用 makecontext() 取得,程序会调用 makecontext() 函数的第二个参数指向的函数，如果 func 函数返回,则恢复 makecontext() 第一个参数指向的上下文第一个参数指向的上下文 context_t 中指向的 uc_link。如果 uc_link 为 NULL,则线程退出。
    setcontext(&context);


    return 0;
}
