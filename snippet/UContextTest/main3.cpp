#include <iostream>

#include <ucontext.h>


void func()
{
    puts("child: In func");
}


int main()
{
    ucontext_t context;

    getcontext(&context);
    context.uc_stack.ss_sp = malloc(8192);
    context.uc_stack.ss_size = 8192;
    context.uc_link = NULL; // 下面的主协程的代码不会执行。

    makecontext(&context, func, 0);
    setcontext(&context);


    puts("main: This will not be printed");
    puts("main: Hello World");


    return 0;
}
