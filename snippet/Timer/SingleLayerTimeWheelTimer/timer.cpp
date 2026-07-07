#include "timer.h"

#include <cassert>


void TimerManager::addTimer(uint32_t timeout, std::function<void()> cb)
{
    // timeout 的单位为 Tick，且必须大于 0。
    // timeout = 1 表示下一次 tick() 执行。
    // timeout = wheelSize 表示经过一整圈后执行。
    assert(timeout > 0);

    // 计算还需要等待多少个完整的一圈。因为第一个 tick 时先移动到槽 1 这个位置了，因此从这个地方开始算第一圈。
    //
    // 例如（wheel = 8）：
    //
    // timeout = 1
    // round = (1 - 1) / 8 = 0
    //
    // timeout = 8
    // round = (8 - 1) / 8 = 0
    //
    // timeout = 9
    // round = (9 - 1) / 8 = 1
    //
    // round 表示：时间轮每次再次经过该槽时，如果 round > 0，则仅减少 round，不执行定时器。
    Timer timer = {
        .round = (timeout - 1) / static_cast<uint32_t>(m_slots.size()),
        .callback = std::move(cb),
    };

    // 计算该定时器所在的槽。
    //
    // 例如（wheel = 8）：
    //
    // 当前槽 current = 0
    //
    // timeout = 1
    // index = (0 + 1) % 8 = 1
    //
    // timeout = 3
    // index = (0 + 3) % 8 = 3
    //
    // timeout = 8
    // index = (0 + 8) % 8 = 0
    //
    // 因为 tick() 会先移动时间轮，再处理当前槽，所以 timeout = 1 正好会在下一次 tick() 执行。
    size_t index = (m_current + timeout) % m_slots.size();
    m_slots[index].emplace_back(std::move(timer));
}

void TimerManager::tick()
{
    // 时间轮先向前转动一个 Tick。
    m_current = (m_current + 1) % m_slots.size();

    // 当前 Tick 对应的槽。
    auto &slot = m_slots[m_current];

    // 遍历当前槽中的所有定时器。
    for (auto it = slot.begin(); it != slot.end();)
    {
        // 还需要等待若干圈。
        if (it->round > 0)
        {
            --it->round;
            ++it;

            continue;
        }

        // 已经超时，执行回调。
        if (it->callback) it->callback();

        // 删除已经完成的定时器，并返回删除后的 it 迭代器。
        it = slot.erase(it);
    }
}
