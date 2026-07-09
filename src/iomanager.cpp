#include "iomanager.h"

#include <cassert>
#include <exception>


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

IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
{
}

IOManager::~IOManager()
{
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
{
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
}

bool IOManager::stopping()
{
}

void IOManager::idle()
{
}

void IOManager::onTimerInsertedAtFront()
{
}

void IOManager::contextResize(size_t size)
{
}
