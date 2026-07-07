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

    // 删除指定下标的定时器。
    void remove(size_t index);

    // 删除堆顶。
    void pop();

    // 执行到期任务。
    void tick();


private:

    // 上滤（Heapify Up）。
    // 从指定节点开始，向父节点方向比较并调整。当某个节点的值小于父节点时，说明该节点破坏了最小堆性质，通过不断与父节点交换，使较小的元素逐渐向上移动，直到恢复父节点 <= 子节点的堆结构。通常用于插入新节点后的堆调整。
    void heapifyUp(size_t index);

    // 下滤（Heapify Down）。
    // 从指定节点开始，向子节点方向比较并调整。当某个节点的值大于子节点时，说明该节点破坏了最小堆性质，选择左右子节点中较小的节点进行交换，使较大的元素逐渐向下移动，直到恢复父节点 <= 子节点的堆结构。通常用于删除节点或替换节点后的堆调整。
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
