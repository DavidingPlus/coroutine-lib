// 单层时间轮实现实现定时器。

#pragma once

#include <functional>
#include <vector>
#include <cstdint>
#include <list>


/**
 * @brief 时间轮中的一个定时器。
 *
 * 单层时间轮并不会按照超时时间排序，而是将定时器放入对应的槽（Slot）中。
 *
 * 当时间轮转动到该槽时：
 *   1. 如果 round > 0，说明还需要再转几圈才能真正超时，只需要将 round 减一。
 *   2. 如果 round == 0，则表示该定时器已经到期，可以执行回调并删除。
 */
struct Timer
{
    // 剩余需要经过的完整轮数。
    uint32_t round = 0;

    // 定时器超时后的回调函数。
    std::function<void()> callback;
};


/**
 * @brief 单层时间轮定时器。
 *
 * 时间轮（Time Wheel）是一种高效的定时器管理方案，它使用一个环形数组模拟钟表，每一个数组元素称为一个槽（Slot），每个槽代表固定的时间间隔（Tick）。
 *
 * 例如：
 *
 *                 [0]
 *            /           \
 *        [59]             [1]
 *       /                   \
 *    [58]                    [2]
 *      |                      |
 *     ...                    ...
 *       \                    /
 *        [31] ...... [30] ..[3]
 *
 * current 表示当前时间轮所指向的槽。
 *
 * 每调用一次 tick()：
 *
 * 1. current 向前移动一个槽；
 * 2. 检查该槽中的所有定时器。
 *
 * 即：
 *
 * current = (current + 1) % slotCount
 *
 * timeout 表示：
 *
 * 定时器还需要经过多少个 Tick 后触发（timeout >= 1）。
 *
 * 一个定时器由：
 *
 * index = (current + timeout) % slotCount
 * round = (timeout - 1) / slotCount
 *
 * 唯一确定其位置。
 *
 * 其中：
 *
 * index 用于确定定时器存放在哪个槽中；
 *
 * round 表示时间轮再次经过该槽时，
 * 还需要等待多少个完整的一圈才能真正触发。
 *
 * 唯一确定其位置。
 *
 * 时间轮最大的特点：
 *
 *  - 添加定时器：O(1)
 *  - 删除定时器：O(1)
 *  - Tick：O(k)，k 为当前槽中的定时器数量
 *
 * 相比升序链表、最小堆等结构，不需要维护全局有序性，因此插入效率非常高。
 */
class TimerManager
{
public:

    explicit TimerManager(size_t slot = 60) : m_slots(slot) {}

    /**
     * @brief 添加一个新的定时器。
     *
     * @param timeout 经过多少个 tick 超时，规定 timeout>=1。
     * @param cb      超时后的回调函数
     */
    void addTimer(uint32_t timeout, std::function<void()> cb);

    /**
     * @brief 推动时间轮向前转动一个 Tick，执行转动后槽中的任务。
     *
     * 当前槽中的所有定时器都会被检查：
     *  - round > 0：说明还需要等待若干圈，仅减少 round。
     *  - round == 0：执行回调，并删除定时器。
     */
    void tick();


private:

    // 时间轮，每一个元素就是一个槽（Slot）。
    std::vector<std::list<Timer>> m_slots;

    // 当前时间轮所指向的槽。
    size_t m_current = 0;
};
