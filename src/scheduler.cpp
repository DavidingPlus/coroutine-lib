#include "scheduler.h"

#include <cassert>
#include <chrono>


// 正在运行的协程调度器。调度器 Scheduler 是进程全局唯一的。每个线程都持有一个线程局部的指向 Scheduler 的指针。
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

    // SetThis(); // 设置当前调度器对象。

    Thread::SetName(m_name); // 设置当前线程的名称为调度器的名称 m_name。

    // 使用主线程当作工作线程，创建协程的主要原因是为了实现更高效的任务调度和管理。
    if (useCaller) // 如果 user_caller 为 true，表示当前线程也要作为一个工作线程使用。
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
    std::lock_guard<std::mutex> lock(m_mutex); // 互斥锁防止共享资源的竞争。

    if (m_stopping) // 如果调度器退出直接报错打印 cerr 后面的话。
    {
        std::cerr << "Scheduler is stopped" << std::endl;


        return;
    }

    assert(m_threads.empty()); // 首先线程池数量为空。
    m_threads.resize(m_threadCount);

    for (size_t i = 0; i < m_threadCount; ++i)
    {
        // 进程全局只有一个调度器，这个调度器管理一个线程池，每个线程都执行同一个调度器的调度循环（run()），共同消费同一个任务队列。
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }

    std::cout << "Scheduler::start() success\n";
}

void Scheduler::stop()
{
    std::cout << "Schedule::stop() starts in thread: " << Thread::GetThreadId() << std::endl;

    // 1. 判断是否已经可以停止。
    if (stopping()) return;

    m_stopping = true;

    // 2. 检查 stop() 是否再正确的线程调用。
    // TODO 如果当前调度器使用 Caller 模式，那么 stop() 必须在调度线程（Caller 线程）里调用；如果不是 Caller 模式，那么 stop() 不能在 Scheduler 工作线程里调用。
    m_useCaller ? assert(GetThis() == this) : assert(GetThis() != this);

    // 3. 唤醒所有睡眠中的线程。

    // 调用 tickle() 的目的唤醒空闲线程或协程，防止 m_scheduler 或其他线程处于永久阻塞在等待任务的状态中
    for (size_t i = 0; i < m_threadCount; ++i) tickle();
    // 唤醒可能处于挂起状态，等待下一个任务的调度的协程。
    if (m_schedulerFiber) tickle();

    // 当只有主线程或调度线程作为工作线程的情况，只能从 stop() 方法开始任务调度。
    if (m_schedulerFiber)
    {
        m_schedulerFiber->resume(); // 开始任务调度。

        std::cout << "m_schedulerFiber ends in thread:" << Thread::GetThreadId() << std::endl;
    }
    // 获取此时的线程通过 swap 不会增加引用计数的方式加入到 thrs，方便下面的 join 保持线程正常退出。
    std::vector<std::shared_ptr<Thread>> thrs;
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        thrs.swap(m_threads);
    }

    for (auto &i : thrs) i->join();

    std::cout << "Schedule::stop() ends in thread:" << Thread::GetThreadId() << std::endl;
}

void Scheduler::tickle()
{
    // 留空。具体代码在子类 io+scheduler。
}

void Scheduler::run()
{
    int threadId = Thread::GetThreadId(); // 获取当前线程的 ID。
    std::cout << "Schedule::run() starts in thread: " << threadId << std::endl;

    // set_hook_enable(true);

    SetThis(); // 设置调度器对象。

    // 新创建的工作线程刚启动时还没有主协程，需要先创建线程的 Main Fiber。后续所有任务协程都会与 Main Fiber 进行上下文切换。注意：这里只创建 Main Fiber，不会创建 Scheduler Fiber。Scheduler Fiber 仅在   useCaller=true 时，为 Caller 线程额外创建。
    if (threadId != m_rootThread) Fiber::GetThis();

    // 创建空闲协程，std::make_shared 是 C++11 引入的一个函数，用于创建 std::shared_ptr 对象。相比于直接使用 std::shared_ptr 构造函数，std::make_shared 更高效且更安全，因为它在单个内存分配中同时分配了控制块和对象，避免了额外的内存分配和指针操作。
    std::shared_ptr<Fiber> idleFiber = std::make_shared<Fiber>(std::bind(&Scheduler::idle, this));

    ScheduleTask task;
    while (true)
    {
        task.reset();
        // 是否需要唤醒其他线程进行任务调度。
        bool tickleMe = false;

        {
            std::lock_guard<std::mutex> lock(m_mutex);

            // 不能直接取第一个任务，因为 Scheduler 支持指定任务运行在指定线程上。
            auto it = m_tasks.begin();
            // 1. 遍历任务队列。
            while (m_tasks.end() != it)
            {
                // 设置了指定的线程 ID，就遍历到指定的线程 ID 再取任务。
                if (-1 != it->thread && threadId != it->thread)
                {
                    ++it;
                    tickleMe = true; // 这个任务虽然不是我的，但是可以被其他线程执行，因此设置为 true。

                    continue;
                }

                // 2. 取出任务。
                assert(it->fiber || it->cb);
                task = *it;
                m_tasks.erase(it);
                ++m_activeThreadCount;

                break; // 这里取到任务的线程就直接 break 所以并没有遍历到队尾。
            }
            // 代码执行到这里，说明已经取走任务了。如果队列中还有任务，那么仍然可以通知其他线程醒来工作。加第二个条件的判断是因为上面的遍历任务队列上来就命中了，直接退出循环，这时 tickleMe 是 false，因此需要两个条件一起判断。
            tickleMe = tickleMe || (it != m_tasks.end());
        }

        // 这里虽然写了唤醒但并没有具体的逻辑代码，具体的在 io+scheduler。
        if (tickleMe) tickle();

        // 3. 执行任务。
        if (task.fiber)
        {
            // resume 协程，resume 返回时此时任务要么执行完了，要么半路 yield，总之任务完成了，活跃线程 -1。
            {
                std::lock_guard<std::mutex> lock(task.fiber->m_mutex);

                if (Fiber::TERMINATE != task.fiber->getState()) task.fiber->resume();
            }

            --m_activeThreadCount; // 线程完成任务后就不再处于活跃状态，而是进入空闲状态，因此需要将活跃线程计数减一。
            task.reset();
        }
        else if (task.cb)
        {
            // 对于函数也应该被调度，具体做法就封装成协程加入调度。
            std::shared_ptr<Fiber> cbFiber = std::make_shared<Fiber>(task.cb);
            {
                std::lock_guard<std::mutex> lock(cbFiber->m_mutex);

                cbFiber->resume(); // 刚创建出来的工作协程状态是 READY，可以被直接 resume()。
            }

            --m_activeThreadCount;
            task.reset();
        }
        // 4. 无任务 -> 执行空闲协程。
        else
        {
            // 系统关闭 -> idle 协程将从死循环跳出并结束 -> 此时的 idle 协程状态为 TERMINATE -> 再次进入将跳出循环并退出 run()。
            if (Fiber::TERMINATE == idleFiber->getState())
            {
                // 如果调度器没有调度任务，那么 idle 协程回不断的 resume/yield，不会结束进入一个忙等待，如果 idle 协程结束了，一定是调度器停止了，直到有任务才执行上面的 if/else，在这里 idleFiber 就是不断的和主协程进行交互的子协程。
                std::cout << "Schedule::run() ends in thread: " << threadId << std::endl;

                break;
            }

            ++m_idleThreadCount;
            idleFiber->resume();
            --m_idleThreadCount;
        }
    }
}

void Scheduler::idle()
{
    while (!stopping())
    {
        std::cout << "Scheduler::idle(), sleeping in thread: " << Thread::GetThreadId() << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1)); // 降低空闲协程在无任务时对 cpu 占用率，避免空转浪费资源。

        Fiber::GetThis()->yield();
    }
}

bool Scheduler::stopping()
{
    // 使用互斥锁的目的因为 m_tasks，mactiveThreadCount 会被多线程竞争所以需要互斥锁来保护资源的访问。
    // 此时这个函数的目的就是为了判断调度器是否退出。在 stop 函数中如果 stopping 为 true 代表调度器已经退出直接返回 return 不进行任何的操作。
    std::lock_guard<std::mutex> lock(m_mutex);


    return m_stopping && m_tasks.empty() && 0 == m_activeThreadCount;
}
