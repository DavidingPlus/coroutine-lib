#include <iostream>

#include <ucontext.h>


constexpr int STACK_SIZE = 1024 * 64;

ucontext_t mainCtx;
ucontext_t workerCtx;

char workerStack[STACK_SIZE];


void worker()
{
    std::cout << "worker: start" << std::endl;

    std::cout << "worker: yield to main" << std::endl;

    swapcontext(&workerCtx, &mainCtx);

    std::cout << "worker: resumed" << std::endl;

    std::cout << "worker: end" << std::endl;
}


int main()
{
    // 获取当前上下文。
    getcontext(&workerCtx);

    // 设置协程栈。
    workerCtx.uc_stack.ss_sp = workerStack;
    workerCtx.uc_stack.ss_size = STACK_SIZE;

    // worker 结束后返回 main。
    workerCtx.uc_link = &mainCtx;

    // 指定入口函数。
    makecontext(&workerCtx, worker, 0);

    std::cout << "main: switch to worker" << std::endl;

    swapcontext(&mainCtx, &workerCtx);

    std::cout << "main: back from worker" << std::endl;

    std::cout << "main: switch again" << std::endl;

    swapcontext(&mainCtx, &workerCtx);

    std::cout << "main: finish" << std::endl;


    return 0;
}
