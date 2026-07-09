#include "iomanager.h"

#include <cassert>
#include <cstring>
#include <exception>

#include <sys/epoll.h>
#include <fcntl.h>


IOManager::FdContext::EventContext &IOManager::FdContext::getEventContext(Event event)
{
    // 判断事件要么是读事件，或者写事件。
    assert(Event::READ == event || Event::WRITE == event);

    switch (event)
    {
        case Event::READ:
            return readEc;
        case Event::WRITE:
            return writeEc;
    }

    throw std::invalid_argument("Unsupported event type");
}

void IOManager::FdContext::resetEventContext(EventContext &ctx)
{
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(Event event)
{
    // 确保 event 是中有指定的事件，否则程序中断。
    assert(static_cast<int>(events) & static_cast<int>(event));

    // 清理该事件，表示不再关注，也就是说，注册 IO 事件是一次性的，如果想持续关注某个 Socket fd 的读写事件，那么每次触发事件后都要重新添加。
    // Event 使用位标志表示多个事件。0000 NONE，0001 READ，0100 WRITE。因此可以通过按位与 ~static_cast<int>(event) 清除指定 bit。
    events = (Event)(static_cast<int>(events) & ~static_cast<int>(event));

    // 获取触发事件对应的上下文信息。
    EventContext &ctx = getEventContext(event);
    // 这个过程就相当于 Scheduler 中把真正要执行的函数放入到任务队列中等线程取出后任务后，协程执行，执行完成后返回主协程继续，执行 run() 方法取任务执行任务(不过可能是不同的线程的协程执行了)。
    ctx.cb ? ctx.scheduler->scheduleLock(&ctx.cb) : ctx.scheduler->scheduleLock(&ctx.fiber);

    // 重置事件上下文。
    resetEventContext(ctx);
}

IOManager::IOManager(size_t threads, bool useCaller, const std::string &name)
    : Scheduler(threads, useCaller, name), TimerManager()
{
    // 创建 epoll fd。成功返回大于 0 的文件描述符，错误返回 -1。
    // 5000，epoll_create 的参数实际上在现代 Linux 内核中已经被忽略，最早版本的 Linux 中，这个参数用于指定 epoll 内部使用的事件表的大小。
    m_epfd = epoll_create(5000);
    assert(m_epfd > 0);

    // 管道 pipe 通常用于进程间通信，但由于线程共享进程的文件描述符表，因此同一进程内的多个线程也可以通过 pipe 通信。IOManager 使用 pipe 并不是为了传递业务数据，而是创建一个可以被 epoll 监听的文件描述符。
    // IOManager 的工作线程通常阻塞在 epoll_wait() 中等待 IO 事件。当其他线程向调度器添加任务时，如果没有唤醒机制，工作线程会一直停留在 epoll_wait()，无法及时执行新加入的任务。因此需要一种能够让 epoll_wait() 主动返回的通知机制。
    // 之所以选择 pipe，是因为 Linux 的 epoll 只能监听文件描述符，而 pipe 本身就是内核提供的 fd 对象。当其他线程向 pipe 写端写入数据时，pipe 读端会产生可读事件，epoll_wait() 检测到该事件后返回，从而唤醒阻塞中的 IOManager 工作线程。
    // pipe 中传递的数据没有业务含义，仅表示“有新的任务或者状态变化，需要重新检查调度队列”。本质上是将线程间通知转换成 epoll 可以感知的 IO 事件。
    //
    // 为什么不使用 mutex、条件变量等传统线程同步机制？
    // 1. mutex（互斥锁）：
    //    - mutex 只能保证多个线程访问共享数据时的互斥性，不能主动唤醒阻塞在 epoll_wait() 中的线程。
    //    - 如果线程正在执行 epoll_wait()，另一个线程释放锁并不会让 epoll_wait() 返回。
    //    - 因此 mutex 适合保护任务队列，而不适合作为 IO 事件循环的唤醒机制。
    // 2. condition_variable（条件变量）：
    //    - 条件变量可以实现线程间等待和唤醒，例如 notify_one() 唤醒 wait()。
    //    - 但是条件变量只能唤醒阻塞在 condition_variable::wait() 上的线程，无法唤醒阻塞在 epoll_wait() 上的线程。
    //    - IOManager 的线程睡眠点是 epoll_wait()，因此条件变量无法直接使用。
    // 3. sleep / 轮询检查：
    //    - 可以通过定时检查任务队列的方式发现新任务，但会造成延迟或者 CPU 空转。
    //    - 不符合高性能事件驱动模型的设计。
    // 4. pipe、eventfd、socketpair：
    //    - 这些机制本身都是文件描述符，可以直接注册到 epoll 中。
    //    - 当事件发生时，epoll_wait() 能够感知并返回，因此适合作为事件循环的唤醒源。
    // 其中 eventfd 是 Linux 专门为事件通知设计的 fd，性能通常优于 pipe；socketpair 支持双向通信；pipe 实现简单、兼容性好，因此很多早期事件框架会采用 pipe。

    // 创建管道 pipe。创建管道的函数规定了 m_tickleFds[0] 是读端，1 是写段。成功返回 0，错误返回 -1。
    int res = pipe(m_tickleFds);
    assert(!res);

    // 将管道的监听注册到 epoll 上。
    epoll_event event;

    // epoll 的触发模式分为水平触发（LT）和边缘触发（ET）：
    // 1. 水平触发（Level Trigger，LT）：
    //    - 关注 fd 当前是否满足条件。
    //    - 只要缓冲区中仍然存在可读数据，epoll 就会持续通知。
    //    - 例如：缓冲区有 100 字节数据，第一次读取 50 字节后仍剩余 50 字节，epoll 仍会继续触发读事件。
    //    - 优点：编程简单，不容易遗漏事件。
    //    - 缺点：可能产生大量重复通知，降低高并发场景下的效率。
    // 2. 边缘触发（Edge Trigger，ET）：
    //    - 关注 fd 状态是否发生变化，只在状态变化时通知一次。
    //    - 例如：缓冲区从 0 字节变为 100 字节时触发一次，读取 50 字节后剩余 50 字节不会再次通知。
    //    - 优点：减少 epoll 通知次数，提高 IO 处理效率。
    //    - 缺点：要求应用程序一次事件内必须尽可能处理完所有数据，否则剩余数据可能不会再次触发事件。
    // 3. ET 模式通常需要配合非阻塞 IO：
    //    - 因为 ET 模式要求循环读取数据，一次把数据读取完，直到 read/write 返回 EAGAIN，表示当前数据已经处理完成。
    //    - 如果 fd 是阻塞模式，当数据读取完后继续调用 read/write，会因为没有数据而阻塞线程。
    //    - 设置非阻塞后，数据处理完成时可以立即返回 EAGAIN，让程序退出当前事件处理，重新进入 epoll_wait 等待下一次事件。
    // 高性能网络模型通常采用：非阻塞 fd + epoll ET + 事件循环。epoll_wait 负责等待事件发生，非阻塞 IO 负责保证事件处理过程不会因为数据不足而阻塞。

    // 设置标志位，并且采用读事件 EPOLLIN 和边缘触发 EPOLLET。
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    // fcntl 是 Linux 提供的文件描述符控制函数，用于修改或获取 fd 的属性。
    // 常用操作包括：
    // 1. F_GETFD / F_SETFD：file descriptor flags
    //    - 文件描述符本身的标志。
    //    - 例如 FD_CLOEXEC，表示进程执行 exec 系列函数时是否自动关闭该 fd。
    // 2. F_GETFL / F_SETFL：file status flags
    //    - 对应的文件状态标志。
    //    - 常见标志包括：
    //      O_NONBLOCK：设置非阻塞模式。
    //      O_APPEND：写文件时追加到文件末尾。
    //      O_ASYNC：启用异步 IO 信号通知。

    // 修改管道文件描述符以非阻塞的方式，配合边缘触发。
    res = fcntl(m_tickleFds[0], F_SETFL, fcntl(m_tickleFds[0], F_GETFL) | O_NONBLOCK);
    assert(!res);

    // 将 m_tickleFds[0] 作为读事件放入到 event 监听集合中。
    res = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    assert(!res);

    // 初始化了一个包含 32 个文件描述符上下文的数组。
    contextResize(32);

    // 启动 Scheduler，开启线程池，准备处理任务。
    start();
}

IOManager::~IOManager()
{
    // 关闭 Scheduler 类中的线程池，让任务全部执行完后线程安全退出。
    stop();

    // 关闭 epoll 的句柄。
    close(m_epfd);

    // 关闭管道读端写端。
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    // 将 fdcontext 文件描述符一个个关闭。
    for (size_t i = 0; i < m_fdContexts.size(); ++i)
    {
        if (m_fdContexts[i]) delete m_fdContexts[i];
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
{
    // 查找 FdContext 对象。
    FdContext *fdCtx = nullptr;

    // 1. 如果说传入的 fd 在数组里面，查找然后初始化 FdContext 的对象。因为没有修改类的内部成员，因此使用读锁。
    std::shared_lock<std::shared_mutex> readLock(m_mutex);
    if (fd < static_cast<int>(m_fdContexts.size()))
    {
        fdCtx = m_fdContexts[fd];

        readLock.unlock();
    }
    // 2. 不存在则重新分配数组的 size 来初始化 FdContext 的对象。因为修改了 m_fdContexts，因此使用写锁。
    else
    {
        readLock.unlock();
        std::unique_lock<std::shared_mutex> writeLock(m_mutex);

        contextResize(fd * 1.5);
        fdCtx = m_fdContexts[fd];
    }

    // 找到或者创建 FdContext 的对象后，为 FdContext 加上互斥锁，确保 FdContext 的状态不会被其他线程修改。
    // 注意：这里的锁是 FdContext 上下文对象自身的，和前面 IOManager 的读写锁不同，注意区分。
    std::lock_guard<std::mutex> lock(fdCtx->mutex);

    // 判断要添加的事件是否已经存在了？是就返回 -1，因为相同的事件不能重复添加。
    if (static_cast<int>(fdCtx->events) & static_cast<int>(event)) return -1;

    // epfd 第一次注册事件，需要 EPOLL_CTL_ADD；如果已经注册了其他事件（例如 READ），再添加 WRITE 时使用 EPOLL_CTL_MOD 修改监听事件。
    int op = static_cast<bool>(fdCtx->events) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    // 我们认为设计 Event 的取值与 EPOLLIN/EPOLLOUT 保持一致，因此可以直接作为 epoll_event.events 的位标志，可以使用按位或运算。
    epevent.events = EPOLLET | static_cast<int>(fdCtx->events) | static_cast<int>(event);
    // epoll_event 的 data 成员是一个用户自定义数据区，内核不会解析其中的内容，而是在事件触发时原样返回。这里将当前 fd 对应的 FdContext 指针保存到 data.ptr 中。当 epoll_wait() 返回事件时，可以直接通过 event.data.ptr 取回对应的 FdContext，从而快速获得该 fd 的读写事件、回调函数、协程以及调度器等信息，无需再根据 fd 去查找。相比只保存 fd（event.data.fd），保存 FdContext* 可以避免一次额外的查表操作，也是 libevent、libuv、muduo 等事件驱动框架的常见做法。
    epevent.data.ptr = fdCtx;

    // 将事件添加到 epoll 中。如果添加失败，打印错误信息并返回 -1。epoll_ctl() 成功返回 0，失败返回 -1。
    if (epoll_ctl(m_epfd, op, fd, &epevent))
    {
        std::cerr << "addEvent::epoll_ctl() failed: " << strerror(errno) << std::endl;


        return -1;
    }

    // 原子计数器，待处理的事件++；
    ++m_pendingEventCount;

    // 更新 FdContext 的 events 成员，记录当前的所有事件。注意 events 可以监听读和写的组合，如果 fdCtx->events 为 none,就相当于直接是 fdCtx->events = event
    fdCtx->events = static_cast<Event>(static_cast<int>(fdCtx->events) | static_cast<int>(event));

    // 设置 FdContext 中的事件上下文。
    FdContext::EventContext &eventCtx = fdCtx->getEventContext(event);
    // 确保 EventContext 中没有其他正在执行的调度器、协程或回调函数，也就是都为空。
    assert(!eventCtx.scheduler && !eventCtx.fiber && !eventCtx.cb);
    // 设置调度器为当前的调度器实例（Scheduler::GetThis()）。FdContext 本身只负责记录 fd 对应的读写事件及其回调信息，并不知道事件触发后应该交给哪个调度器执行。因此在注册事件时，需要记录当前线程正在运行的 Scheduler（通过 Scheduler::GetThis() 获取）。当 epoll_wait() 检测到该事件发生后，triggerEvent() 会通过保存的 scheduler 将对应的协程或回调重新加入调度队列，从而由正确的调度器负责恢复协程或执行回调。
    // 为什么不直接使用 this？因为这里需要保存的是"当前线程实际运行的调度器"这一概念，而不是单纯保存当前 IOManager 对象。对于 IOManager 而言，this 和 Scheduler::GetThis() 在正常情况下通常指向同一个对象（IOManager 继承自 Scheduler），直接使用 this 也能够正常工作。但整个框架统一通过 Scheduler::GetThis() 获取当前线程绑定的调度器，更符合调度器的设计思想，也避免代码依赖具体的对象指针，保持接口风格一致。
    eventCtx.scheduler = Scheduler::GetThis();

    // 如果提供了回调函数 cb，则将其保存到 EventContext 中；否则，将当前正在运行的协程保存到 EventContext 中，并确保协程的状态是正在运行。
    if (cb)
    {
        eventCtx.cb.swap(cb);
    }
    else
    {
        // 需要确保存在主协程。
        eventCtx.fiber = Fiber::GetThis();
        assert(Fiber::RUNNING == eventCtx.fiber->getState());
    }


    return 0;
}

bool IOManager::delEvent(int fd, Event event)
{
}

bool IOManager::cancelEvent(int fd, Event event)
{
}

bool IOManager::cancelAll(int fd)
{
}

void IOManager::tickle()
{
    // 检查当前是否有线程处于空闲状态。如果没有空闲线程，函数直接返回，不执行后续操作。
    if (!hasIdleThreads()) return;

    // 如果有空闲线程，函数会向管道 m_tickleFds[1] 写入一个字符 "T"。这个写操作的目的是向等待在 m_tickleFds[0]（管道的另一端）的线程发送一个信号，通知它有新任务可以处理了。
    int res = write(m_tickleFds[1], "T", 1);
    assert(1 == res);
}

bool IOManager::stopping()
{
    // 重写了 Scheduler 的 stopping()。具体来说，它会检查定时器、挂起事件以及调度器状态，以决定是否可以安全地停止运行。需要注意的是：Scheduler 也有一个 stopping() 函数，但是因为重写的原因实际业务中真正执行的是 IOmanager 的函数，Scheduler 的 stopping() 函数只是对 Scheduler 类需要确保任务数量等于 0，还有是否需要终止的成员变量，活跃线程总和是否为 0 做一个判断。
    // 没有定时器超时，待处理的事件数量为 0 并且 Scheduler::stopping()。
    return ~0ull == getNextTimer() && 0 == m_pendingEventCount && Scheduler::stopping();
}

void IOManager::idle()
{
}

void IOManager::contextResize(size_t size)
{
    // 调整m_fdContexts的大小。
    m_fdContexts.resize(size);

    //  遍历 m_fdContexts 向量，初始化尚未初始化的 FdContext 对象
    for (size_t i = 0; i < m_fdContexts.size(); ++i)
    {
        if (!m_fdContexts[i])
        {
            m_fdContexts[i] = new FdContext();

            // 将文件描述符的编号 i 赋值给 fd。
            m_fdContexts[i]->fd = i;
        }
    }
}
