#include "timer.h"

#include "config.h"

#include <iostream>
#include <cassert>


bool Timer::cancel()
{
    // 写锁互斥锁保护 TimerManager 对象。
    std::unique_lock<std::shared_mutex> writeLock(m_manager->m_mutex);

    // 先标记 m_cb 为空，表示这个定时器失效了，不能再执行回调。即使后续删除失败了，Timer 也不会执行。
    if (!m_cb)
    {
        if (COROUTINE_CONFIG_DEBUG) std::cout << "Timer::cancel(): m_cb is already nullptr" << std::endl;


        return false;
    }
    else
    {
        m_cb = nullptr;
    }

    // 从定时管理器中找到需要删除的定时器。
    auto it = m_manager->m_timers.find(shared_from_this());
    // 删除定时器。
    if (m_manager->m_timers.end() != it) m_manager->m_timers.erase(it);


    return true;
}

bool Timer::refresh()
{
    std::unique_lock<std::shared_mutex> writeLock(m_manager->m_mutex);

    if (!m_cb)
    {
        if (COROUTINE_CONFIG_DEBUG) std::cout << "Timer::refresh(): m_cb is nullptr" << std::endl;


        return false;
    }

    // 需要先从 TimerManager 中删除当前 Timer。因为 m_next 是 set 的排序依据，直接修改会破坏容器有序性。修改超时时间后，需要重新插入。
    // 在定时器集合中查找当前定时器。
    auto it = m_manager->m_timers.find(shared_from_this());
    // 检查定时器是否存在。
    if (m_manager->m_timers.end() == it) return false;

    // 删除当前定时器并更新超时时间。
    m_manager->m_timers.erase(it);
    m_next = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_ms);
    // 并且将新的定时器加入到定时器管理类中。
    m_manager->m_timers.insert(shared_from_this());


    return true;
}

bool Timer::reset(uint64_t ms, bool fromNow)
{
    // 检查是否要重置。若和原来的一摸一样，代表不需要重置。
    if (ms == m_ms && !fromNow) return true;

    // 同理先删再插。
    {
        std::unique_lock<std::shared_mutex> writeLock(m_manager->m_mutex);

        if (!m_cb) // 如果为空，说明该定时器已经被取消或未初始化，因此无法重置。
        {
            if (COROUTINE_CONFIG_DEBUG) std::cout << "Timer::reset(): m_cb is nullptr" << std::endl;


            return false;
        }

        // 否则就是定时器已经初始化了。
        auto it = m_manager->m_timers.find(shared_from_this());
        if (m_manager->m_timers.end() == it) return false;
        // 找到删除。
        m_manager->m_timers.erase(it);
    }

    // 重新插入。如果为 true 则重新计算超时时间，为 false 就需要上一次的起点开始。
    auto start = fromNow ? std::chrono::steady_clock::now() : m_next - std::chrono::milliseconds(m_ms);
    m_ms = ms;
    m_next = start + std::chrono::milliseconds(m_ms);

    // addTimer 里面会处理加锁的逻辑，因此这里不用受锁的生命周期管理，否则可能死锁。
    m_manager->addTimer(shared_from_this());


    return true;
}

bool Timer::Comparator::operator()(const std::shared_ptr<Timer> &lhs, const std::shared_ptr<Timer> &rhs) const
{
    assert(lhs != nullptr && rhs != nullptr);


    return lhs->m_next < rhs->m_next;
}

TimerManager::TimerManager()
{
}

TimerManager::~TimerManager()
{
}

std::shared_ptr<Timer> TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring)
{
}

std::shared_ptr<Timer> TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weakCond, bool recurring)
{
}

uint64_t TimerManager::getNextTimer()
{
}

void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs)
{
}

bool TimerManager::hasTimer()
{
}

void TimerManager::addTimer(std::shared_ptr<Timer> timer)
{
}
