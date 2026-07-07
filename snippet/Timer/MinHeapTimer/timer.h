// 最小堆实现定时器。

#pragma once

#include <functional>
#include <vector>
#include <cstdint>
#include <unordered_map>


class Timer
{
    friend class TimerManager;

public:

    using Callback = std::function<void()>;

    explicit Timer(uint64_t expire, Callback cb, uint64_t id)
        : m_expire(expire), m_callback(std::move(cb)), m_id(id) {}

private:

    // 超时时间（ms）。
    uint64_t m_expire;

    // 超时回调。
    Callback m_callback;

    // 定时器唯一标识 ID，用于外部程序高效查询 Timer 在最小堆中的位置。
    uint64_t m_id;
};


class TimerManager
{

public:

    using TimerId = uint64_t;

    TimerManager() = default;

    virtual ~TimerManager() = default;

    // 插入定时器。
    TimerId add(uint64_t expire, Timer::Callback cb);

    // 根据 Timer ID 删除定时器。
    void cancel(TimerId id);

    // 执行到期任务。
    void tick();


private:

    // 上滤（Heapify Up）。
    // 从指定节点开始，向父节点方向比较并调整。当某个节点的值小于父节点时，说明该节点破坏了最小堆性质，通过不断与父节点交换，使较小的元素逐渐向上移动，直到恢复父节点 <= 子节点的堆结构。通常用于插入新节点后的堆调整。
    void heapifyUp(size_t index);

    // 下滤（Heapify Down）。
    // 从指定节点开始，向子节点方向比较并调整。当某个节点的值大于子节点时，说明该节点破坏了最小堆性质，选择左右子节点中较小的节点进行交换，使较大的元素逐渐向下移动，直到恢复父节点 <= 子节点的堆结构。通常用于删除节点或替换节点后的堆调整。
    void heapifyDown(size_t index);

    // 根据 m_heap 的下标交换两个结点，并维护映射 m_timerIndex 结构。
    void swapNode(size_t lhs, size_t rhs);

    // 删除堆中指定位置的 Timer。使用最后一个节点覆盖删除位置，并根据新的节点值选择上滤或下滤恢复堆性质。
    void remove(size_t index);

    // 删除堆顶。
    void pop();


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

    // 映射 Timer ID 到最小堆中的下标。用于 O(1) 查找 Timer 所在位置，实现快速删除。
    std::unordered_map<uint64_t, size_t> m_timerIndex;

    // 定时器的下一个 Id。
    TimerId m_nextId = 1;
};
