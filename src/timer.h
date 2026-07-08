#ifndef _COROUTINE_TIMER_H_
#define _COROUTINE_TIMER_H_

#include <memory>
#include <functional>
#include <chrono>
#include <mutex>
#include <set>
#include <shared_mutex>


class TimerManager;


// Timer 使用 shared_ptr 管理生命周期。Timer 在 refresh/reset 时会从 TimerManager 的容器中删除并重新插入，但不会创建新的 Timer 对象。shared_ptr 可以保证 Timer 在被 TimerManager 移除后，如果仍有外部引用，对象不会立即析构，避免出现悬空指针问题。
class Timer : public std::enable_shared_from_this<Timer>
{
    friend class TimerManager;

public:

    // 从时间堆中删除 timer。
    bool cancel();

    // 刷新 timer。保持原来的超时时间间隔不变，仅将下一次触发时间重新设置为当前时间 + m_ms。常用于延长定时器生命周期，例如连接保活。
    bool refresh();

    // 重设 timer 的超时时间。ms 表示新的超时间隔，fromNow 表示是否从当前时间开始重新计算：true：从当前时间开始计算新的超时时间；false：保持原来的起始时间，仅修改时间间隔。可用于动态调整定时器周期。
    bool reset(uint64_t ms, bool fromNow);


private:

    Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager)
        : m_ms(ms), m_cb(std::move(cb)), m_recurring(recurring), m_manager(manager) { m_next = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_ms); }


private:

    // 是否循环。
    bool m_recurring = false;

    // 定时器执行间隔（ms），用于循环定时器中重新计算下一次触发时间。对于单次定时器就是超时时间。
    uint64_t m_ms = 0;

    // 下一次触发的绝对时间。
    // 使用 steady_clock 而不是 system_clock。steady_clock 是单调递增时钟，不受系统时间修改（如手动调时、NTP 校时）影响，适合用于定时器、超时检测等场景；system_clock 表示真实系统时间，更适合日志、日期时间等用途。
    std::chrono::time_point<std::chrono::steady_clock> m_next;

    // 超时时触发的回调函数。
    std::function<void()> m_cb;

    // 管理此 timer 的管理器。
    TimerManager *m_manager = nullptr;


private:

    // 实现最小堆的比较函数，⽤于⽐较两个 Timer 对象，⽐较的依据是绝对超时时间。
    struct Comparator
    {
        bool operator()(const std::shared_ptr<Timer> &lhs, const std::shared_ptr<Timer> &rhs) const;
    };
};


class TimerManager
{
    friend class Timer;

public:

    TimerManager() = default;

    virtual ~TimerManager() = default;

    // 添加 timer。ms 定时器执行间隔时间。cb 定时器回调函数。recurring 是否循环定时器。
    // 将其加入 TimerManager 管理。返回 shared_ptr，方便用户后续调用 cancel、refresh、reset 等接口操作该 Timer。
    std::shared_ptr<Timer> addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

    // 添加条件 timer。weakCond 条件。
    std::shared_ptr<Timer> addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weakCond, bool recurring = false);

    // 拿到堆中最近的超时时间。
    uint64_t getNextTimer();

    // 取出所有超时定时器的回调函数。
    void listExpiredCb(std::vector<std::function<void()>> &cbs);

    // 堆中是否有定时器 timer。
    bool hasTimer();


protected:

    // 当一个最早的 timer 加入到堆中 -> 调用该函数。
    virtual void onTimerInsertedAtFront() {}

    // 添加 timer。这个私有函数是方便内部和 Timer 使用的，外部使用共有函数的那个重载版本。
    void addTimer(std::shared_ptr<Timer> timer);


private:

    std::shared_mutex m_mutex;

    // 时间堆。存储所有的 Timer 对象，并使用 Timer::Comparator 进行排序，确保最早超时的 Timer 在最前面。
    // 使用 std::set（底层是红黑树，这里是拿来模拟最小堆）而不是 std::priority_queue（底层是堆）。两者都能 O(logN) 插入、O(1) 获取最早超时的 Timer（begin()/top()），但 std::set 支持查找、删除和重新插入任意 Timer，便于实现 cancel()、refresh()、reset() 等操作，而 priority_queue 只能操作堆顶元素。
    std::set<std::shared_ptr<Timer>, Timer::Comparator> m_timers;

    // 标识当前最早超时 Timer 是否已经触发过唤醒通知。当新的 Timer 成为最早超时定时器时，说明它比原来的 Timer 更早触发。此时可能需要唤醒正在等待超时的调度线程，重新计算等待时间，因此有更紧急的任务需要处理，不能只关注原来的任务。通过 onTimerInsertedAtFront() 唤醒调度线程，唤醒的线程会重新调用 getNextTimer() 函数。在调度线程重新调用 getNextTimer() 之前，只允许触发一次唤醒，避免重复通知。
    bool m_tickled = false;
};


#endif
