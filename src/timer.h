#ifndef _COROUTINE_TIMER_H_
#define _COROUTINE_TIMER_H_

#include <memory>
#include <functional>
#include <chrono>
#include <mutex>
#include <set>
#include <shared_mutex>


class TimerManager;


class Timer : public std::enable_shared_from_this<Timer>
{
    friend class TimerManager;

public:

    // 从时间堆中删除 timer。
    bool cancel();

    // 刷新 timer。
    bool refresh();

    // 重设 timer 的超时时间。ms 定时器执行间隔时间（ms），fromNow 是否从当前时间开始计算。
    bool reset(uint64_t ms, bool fromNow);


private:

    Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager);


private:

    // 是否循环。
    bool m_recurring = false;

    // 超时时间。
    uint64_t m_ms = 0;

    // 绝对超时时间。
    // 使用 steady_clock 而不是 system_clock。steady_clock 是单调递增时钟，不受系统时间修改（如手动调时、NTP 校时）影响，适合用于定时器、超时检测等场景；system_clock 表示真实系统时间，更适合日志、日期时间等用途。
    std::chrono::time_point<std::chrono::steady_clock> m_next;
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

    TimerManager();

    virtual ~TimerManager();

    // 添加 timer。ms 定时器执行间隔时间。cb 定时器回调函数。recurring 是否循环定时器。
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

    // 添加 timer。
    void addTimer(std::shared_ptr<Timer> timer);


private:

    std::shared_mutex m_mutex;

    // 时间堆。存储所有的 Timer 对象，并使用 Timer::Comparator 进行排序，确保最早超时的 Timer 在最前面。
    // 使用 std::set（底层是红黑树，这里是拿来模拟最小堆）而不是 std::priority_queue（底层是堆）。两者都能 O(logN) 插入、O(1) 获取最早超时的 Timer（begin()/top()），但 std::set 支持查找、删除和重新插入任意 Timer，便于实现 cancel()、refresh()、reset() 等操作，而 priority_queue 只能操作堆顶元素。
    std::set<std::shared_ptr<Timer>, Timer::Comparator> m_timers;

    // 在下次 getNextTimer() 执行前 onTimerInsertedAtFront() 是否已经被触发了 -> 在此过程中   onTimerInsertedAtFront() 只执行一次。防止重复调用。
    bool m_tickled = false;
};


#endif
