#include "fiber.h"


// 正在运行的协程。
static thread_local Fiber *runningFiber = nullptr;
// 主协程。
static thread_local std::shared_ptr<Fiber> mainFiber = nullptr;
// 调度协程。
static thread_local Fiber *schedulerFiber = nullptr;

// 全局协程 ID 计数器。
static std::atomic<uint64_t> fiberId{0};
// 全局活跃协程数量计数器。
static std::atomic<uint64_t> ActiveFiberCount{0};


// TODO
Fiber::Fiber()
{
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool runInScheduler)
{
}

Fiber::~Fiber()
{
}

void Fiber::reset(std::function<void()> cb)
{
}

void Fiber::resume()
{
}

void Fiber::yield()
{
}

void Fiber::SetThis(Fiber *f)
{
    runningFiber = f;
}

std::shared_ptr<Fiber> Fiber::GetThis()
{
}

void Fiber::SetSchedulerFiber(Fiber *f)
{
    schedulerFiber = f;
}

uint64_t Fiber::GetFiberId()
{
    if (runningFiber) return runningFiber->getId();

    return (uint64_t)-1; // 返回 -1，并且是 (uint64_t)-1 那就会转换成 UINT64_MAX，用来表示错误的情况。
}

void Fiber::MainFunc()
{
}
