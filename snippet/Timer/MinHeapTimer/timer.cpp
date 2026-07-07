#include "timer.h"


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
        size_t leftChild = TimerManager::getLeftChildIndex(index);
        size_t rightChild = TimerManager::getRightChildIndex(index);

        auto &node = m_heap[leftChild].m_expire < m_heap[rightChild].m_expire ? m_heap[leftChild] : m_heap[rightChild];

        // TODO
    }
}
