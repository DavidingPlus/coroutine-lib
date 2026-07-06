#include "scheduler.h"

#include <cassert>


// 正在运行的协程调度器。
static thread_local Scheduler *t_scheduler = nullptr;


Scheduler *Scheduler::GetThis()
{
    return t_scheduler;
}

void Scheduler::SetThis()
{
    t_scheduler = this;
}

Scheduler::Scheduler(size_t threads, bool useCaller, const std::string &name)
    : m_useCaller(useCaller), m_name(name)
{
    assert(threads > 0 && Scheduler::GetThis() == nullptr); // 首先判断线程的数量是否大于 0，并且调度器的对象是否是空指针，是就调用 setThis() 进行设置。

    SetThis(); // 设置当前调度器对象。

    Thread::SetName(m_name); // 设置当前线程的名称为调度器的名称 m_name。

    // 使用主线程当作工作线程，创建协程的主要原因是为了实现更高效的任务调度和管理。
    if (useCaller) // 如果user_caller为true，表示当前线程也要作为一个工作线程使用。
    {
        --threads; // 因为主线程作为了工作线程所以需要额外创建的线程数量 --。

        // 创建主协程。
        Fiber::GetThis();

        // 创建调度协程。调度协程显然是 false -> 协程退出后将返回主协程。
        m_schedulerFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false));
        // 设置协程的调度器对象。
        Fiber::SetSchedulerFiber(m_schedulerFiber.get());

        // 获取主线程 ID。
        m_rootThread = Thread::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    }

    m_threadCount = threads; // 将剩余的线程数量（即总线程数量减去是否使用调用者线程）赋值给 m_threadCount。

    std::cout << "Scheduler::Scheduler() success\n";
}

Scheduler::~Scheduler()
{
    assert(true == stopping()); // 判断调度器是否终止。

    // 获取调度器的对象。
    if (this == GetThis())
    {
        t_scheduler = nullptr; // 将其设置为 nullptr 防止悬空指针。
    }

    std::cout << "Scheduler::~Scheduler() success\n";
}

void Scheduler::start()
{
}

void Scheduler::stop()
{
}

void Scheduler::tickle()
{
}

void Scheduler::run()
{
}

void Scheduler::idle()
{
}

bool Scheduler::stopping()
{
}
