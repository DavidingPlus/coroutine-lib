#include "fiber.h"

#include <cassert>


// 正在运行的协程。
static thread_local Fiber *t_fiber = nullptr;
// 主协程。
static thread_local std::shared_ptr<Fiber> t_threadFiber = nullptr;
// 调度协程。
static thread_local Fiber *t_schedulerFiber = nullptr;

// 全局协程 ID 计数器。
static std::atomic<uint64_t> s_fiberId{0};
// 全局活跃协程数量计数器。
static std::atomic<uint64_t> s_fiberCount{0};


// 作用：创建主协程。设置状态，初始化上下文，并分配 ID。
Fiber::Fiber()
{
    SetThis(this);     // 在 getThis 中使用了无参的 Fiber 来构造 t_fiber。
    m_state = RUNNING; // 设置协程的状态为可运行。

    if (getcontext(&m_ctx))
    {
        std::cerr << "Fiber() failed\n";

        pthread_exit(nullptr);
    }

    m_id = s_fiberId++; // 分配 id，协程 id 从 0 开始，用完加 1。
    ++s_fiberCount;     // 活跃的协程数量 +1。

    std::cout << "Fiber(): main id = " << m_id << std::endl;
}

// 作用：创建一个新协程，初始化回调函数，栈的大小和状态。分配栈空间，并通过 make 修改上下文当 set 或 swap 激活 ucontext_t  m_ctx 上下文时候会执行 make 第二个参数的函数。
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool runInScheduler)
    : m_cb(cb), m_runInScheduler(runInScheduler)
{
    m_state = READY; // 初始化状态。

    // 分配协程栈空间。
    m_stackSize = stacksize ? stacksize : 128000;
    m_stack = malloc(m_stackSize);

    if (getcontext(&m_ctx))
    {
        std::cerr << "Fiber(std::function<void()> cb, size_t stacksize, bool run_in_scheduler) failed\n";

        pthread_exit(nullptr);
    }

    m_ctx.uc_link = nullptr; // 这里因为没有设置了后继所以在运行完 mainfunc 后协程退出，会调用一次 yield 返回主协程。
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stackSize;
    makecontext(&m_ctx, &Fiber::MainFunc, 0);

    m_id = s_fiberId++; // 分配 id，协程 id 从 0 开始，用完加 1。
    ++s_fiberCount;     // 活跃的协程数量 +1。

    std::cout << "Fiber(): child id = " << m_id << std::endl;
}

Fiber::~Fiber()
{
    --s_fiberCount; // 减少活跃协程计数器。

    if (m_stack)
    {
        free(m_stack);
        m_stack = nullptr;
    }

    std::cout << "~Fiber(): id = " << m_id << std::endl;
}

// 作用：重置协程的回调函数，并重新设置上下文，使用与将协程从 TERMINATE 状态重置 READY。
void Fiber::reset(std::function<void()> cb)
{
    assert(nullptr != m_stack && TERMINATE == m_state);

    m_state = READY;
    m_cb = cb;

    if (getcontext(&m_ctx))
    {
        std::cerr << "reset() failed\n";

        pthread_exit(nullptr);
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stackSize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
}

// 作用：将协程的状态设置为 running，并恢复协程的执行。如果 m_runInScheduler 为 true，则将上下文切换到调度协程；否则，切换到主线程的协程。
void Fiber::resume()
{
    assert(READY == m_state);

    m_state = RUNNING;

    // 这里的切换就相当于非对称协程函数那个当 a 执行完成后会将执行权交给 b。
    if (m_runInScheduler)
    {
        SetThis(this); // 切换为目前工作的协程。
        if (swapcontext(&(t_schedulerFiber->m_ctx), &m_ctx))
        {
            std::cerr << "resume() to t_schedulerFiber failed\n";

            pthread_exit(nullptr);
        }
    }
    else
    {
        SetThis(this);
        if (swapcontext(&(t_threadFiber->m_ctx), &m_ctx))
        {

            std::cerr << "resume() to t_schedulerFiber failed\n";

            pthread_exit(nullptr);
        }
    }
}

void Fiber::yield()
{
    assert(m_state == RUNNING || m_state == TERMINATE);

    if (TERMINATE != m_state) m_state = READY;

    if (m_runInScheduler)
    {
        SetThis(t_schedulerFiber);
        if (swapcontext(&m_ctx, &(t_schedulerFiber->m_ctx)))
        {
            std::cerr << "yield() to to t_schedulerFiber failed\n";

            pthread_exit(nullptr);
        }
    }
    else
    {
        SetThis(t_threadFiber.get());
        if (swapcontext(&m_ctx, &(t_threadFiber->m_ctx)))
        {
            std::cerr << "yield() to t_threadFiber failed\n";

            pthread_exit(nullptr);
        }
    }
}

void Fiber::SetThis(Fiber *f)
{
    t_fiber = f;
}

std::shared_ptr<Fiber> Fiber::GetThis()
{
    // 如果有正在运行的协程就直接返回。
    if (t_fiber) return t_fiber->shared_from_this();

    // 如果没有就返回主协程。
    t_threadFiber = std::shared_ptr<Fiber>(new Fiber()); // 不能用 t_threadFiber = std::make_shared<Fiber>()，因为 Fiber() 的默认构造是私有的，不能从外部调用。
    t_schedulerFiber = t_threadFiber.get();              // 除非主动设置，主协程默认为调度协程。

    assert(t_fiber == t_threadFiber.get()); // 用于判断，t_fiber 是否等于 main_fiber。是继续执行，否则程序终止。


    return t_fiber->shared_from_this();
}

void Fiber::SetSchedulerFiber(Fiber *f)
{
    t_schedulerFiber = f;
}

uint64_t Fiber::GetFiberId()
{
    if (t_fiber) return t_fiber->getId();

    return (uint64_t)-1; // 返回 -1，并且是 (uint64_t)-1 那就会转换成 UINT64_MAX，用来表示错误的情况。
}

void Fiber::MainFunc()
{
    std::shared_ptr<Fiber> cur = GetThis(); // GetThis() 的 shared_from_this() ⽅法让引⽤计数加 1。
    assert(nullptr != cur);

    cur->m_cb();
    cur->m_cb = nullptr;
    cur->m_state = TERMINATE;

    // 运行完毕 -> 让出执行权。
    auto rawPtr = cur.get();
    cur.reset(); // 这里留意一个细节，就是重置的 cb 回调函数就希望它指向 nullptr 因为方便其他线程再次调用这个协程对象。
    rawPtr->yield();
}
