#include <iostream>

#include <ucontext.h>


void func(void *arg)
{
    puts("child: 1");
    puts("child: 11");
    puts("child: 111");
    puts("child: 1111");
}


// 简单说来，getcontext() 获取当前上下文，setcontext() 设置当前上下文，swapcontext() 切换上下文，makecontext() 创建一个新的上下文。
int main()
{
    puts("main:");

    char stack[1024 * 128];
    ucontext_t child, main;

    getcontext(&child);                     // 获取当前上下文。
    child.uc_stack.ss_sp = stack;           // 指定栈空间。
    child.uc_stack.ss_size = sizeof(stack); // 指定栈空间大小。
    child.uc_stack.ss_flags = 0;
    child.uc_link = &main; // 设置后继上下文。
    // child.uc_link = NULL;

    // makecontext() 修改通过 getcontext() 取得的上下文 ucp(这意味着调用 makecontext() 前必须先调用getcontext())。然后给该上下文指定一个栈空间 ucp->stack，设置后继的上下文 ucp->uc_link。
    makecontext(&child, (void (*)(void))func, 0); // 修改上下文指向 func 函数。

    // swapcontext() 保存当前上下文到 oucp 结构体中，然后激活 upc 上下文。
    // 如果执行成功，getcontext() 返回 0，setcontext() 和 swapcontext() 不返回；如果执行失败，getcontext(),setcontext(),swapcontext() 返回 -1，并设置对于的 errno。
    swapcontext(&main, &child); // 切换到 child 上下文，保存当前上下文到 main。

    puts("main: 11111"); // 如果设置了后继上下文，func 函数指向完后会返回此处。


    return 0;
}
