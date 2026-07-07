// 最小堆实现定时器。

#pragma once

#include <functional>
#include <vector>
#include <cstdint>


class Timer
{
    friend class TimerManager;

public:

    using Callback = std::function<void()>;

    explicit Timer(uint64_t expire, Callback cb)
        : m_expire(expire), m_callback(std::move(cb)) {}

private:

    // 超时时间（ms）。
    uint64_t m_expire;

    // 超时回调。
    Callback m_callback;
};


class TimerManager
{

public:

    TimerManager() = default;

    virtual ~TimerManager() = default;

    // 插入。
    void add(const Timer &timer);

    // TODO 删除堆顶。目前只支持删除堆顶的元素。
    void pop();

    // 执行到期任务。
    void tick();


private:

    // 上滤（Heapify Up）。
    void heapifyUp(size_t index);

    // 下滤（Heapify Down）。
    void heapifyDown(size_t index);


private:

    // 计算父结点下标。
    static size_t getParentIndex(size_t index);

    // 计算左孩子下标。
    static size_t getLeftChildIndex(size_t index);

    // 计算右孩子下标。
    static size_t getRightChildIndex(size_t index);


private:

    // 使用数组模拟完全二叉树。
    std::vector<Timer> m_heap;
};
