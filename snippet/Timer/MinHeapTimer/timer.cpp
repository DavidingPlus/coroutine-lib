#include "timer.h"

#include "config.h"

#include <iostream>
#include <chrono>


size_t TimerManager::getParentIndex(size_t index) { return (index - 1) / 2; }

size_t TimerManager::getLeftChildIndex(size_t index) { return index * 2 + 1; }

size_t TimerManager::getRightChildIndex(size_t index) { return index * 2 + 2; }


void TimerManager::add(const Timer &timer)
{
    m_heap.emplace_back(timer);

    // 新插入节点只可能破坏它与父节点之间的堆序。每次交换后，原父节点下沉，新父结点的值只会更小，因此会小于自己的两个孩子，因此下面的子树依旧满足堆性质；唯一可能出现问题的是新的父子关系，所以继续向上调整即可。
    heapifyUp(m_heap.size() - 1);
}

void TimerManager::pop()
{
    // 为了维持完全二叉树的性质，我们把数组末端的值覆盖到数组首部，然后删除数组末端的元素，最后使用 heapifyDown() 调整即可。
    // 删除堆顶元素后，将最后一个节点移动到根节点，此时可能不满足最小堆性质。由于问题只可能出现在新根节点与子节点之间，因此从根节点开始向下调整。每次选择左右孩子中较小的节点与当前节点交换，这样能保证交换后的根结点是三者中最小的。交换后的子节点值变大了，它和它的子树可能不符合要求，持续调整满足最小堆要求。
    std::swap(m_heap.front(), m_heap.back());
    m_heap.pop_back();

    heapifyDown(0);
}

void TimerManager::tick()
{
    // 获取当前时间，用于判断定时器是否到期。
    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

    if (COROUTINE_CONFIG_DEBUG) std::cout << "TimerManager::tick() now: " << now << std::endl;

    while (!m_heap.empty())
    {
        // 最小堆的堆顶永远保存最早到期的定时器。如果堆顶定时器还未到期，说明其它定时器也不可能到期，直接退出。
        if (now < m_heap[0].m_expire) break;

        // 保存堆顶定时器。先删除定时器再执行回调，避免回调函数内部修改 TimerManager 导致堆结构变化。
        Timer timer = m_heap[0];

        // 删除已经到期的定时器，并通过 heapifyDown() 恢复最小堆性质。
        pop();

        // 执行定时器回调。
        if (timer.m_callback) timer.m_callback();
    }
}

void TimerManager::heapifyUp(size_t index)
{
    while (index)
    {
        size_t parent = TimerManager::getParentIndex(index);

        // 注意边界条件，相同的时候也直接返回。
        if (m_heap[parent].m_expire <= m_heap[index].m_expire) break;

        std::swap(m_heap[parent], m_heap[index]);

        index = parent;
    }
}

void TimerManager::heapifyDown(size_t index)
{
    while (index < m_heap.size())
    {
        size_t left = getLeftChildIndex(index);
        size_t right = getRightChildIndex(index);

        // 完全二叉树中，没有左孩子，一定没有右孩子，说明当前节点是叶子节点。
        if (left >= m_heap.size()) break;

        // 默认左孩子较小，选择左孩子。
        size_t smaller = left;

        // 如果右孩子存在，并且右孩子更小，则选择右孩子。
        if (right < m_heap.size() && m_heap[right].m_expire < m_heap[left].m_expire) smaller = right;

        // 已经满足最小堆性质。
        if (m_heap[index].m_expire <= m_heap[smaller].m_expire) break;

        // 当前节点较大，与较小孩子交换。
        std::swap(m_heap[index], m_heap[smaller]);

        index = smaller;
    }
}
