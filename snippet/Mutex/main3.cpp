/*
 * std::shared_mutex（读写锁）
 *
 * 与 std::mutex 不同，shared_mutex 区分"读"和"写"两种加锁方式：
 *
 * 1. 读（共享锁）
 *    使用 std::shared_lock<std::shared_mutex>
 *    - 允许多个线程同时获取锁
 *    - 适用于只读操作，不允许修改共享数据
 *
 * 2. 写（独占锁）
 *    使用 std::unique_lock<std::shared_mutex>
 *    - 同一时刻只能有一个线程持有
 *    - 写期间会阻塞所有读线程和写线程
 *
 * 锁兼容关系：
 * ┌────────────┬────────────┬──────────┐
 * │ 当前持锁   │ 新请求     │ 是否允许 │
 * ├────────────┼────────────┼──────────┤
 * │ Reader   │ Reader   │    √    │
 * │ Reader   │ Writer   │    ×    │
 * │ Writer   │ Reader   │    ×    │
 * │ Writer   │ Writer   │    ×    │
 * └────────────┴────────────┴──────────┘
 *
 * 常见用法：
 *
 * std::shared_mutex mtx;
 *
 * // 读操作
 * {
 *     std::shared_lock<std::shared_mutex> lock(mtx);
 *     // 读取共享数据
 * }
 *
 * // 写操作
 * {
 *     std::unique_lock<std::shared_mutex> lock(mtx);
 *     // 修改共享数据
 * }
 *
 * 适用场景：
 * - 读多写少（如缓存、配置、查询等）
 * - 多个线程可以并发读取，提高并发性能
 * - 修改数据时仍能保证线程安全
 */
#include <iostream>
#include <chrono>
#include <shared_mutex>
#include <mutex>
#include <thread>


std::shared_mutex g_mutex;

int g_value = 0;


// 读线程。
void reader(int id)
{
    for (int i = 0; i < 5; ++i)
    {
        {
            std::shared_lock<std::shared_mutex> lock(g_mutex);

            std::cout << "[Reader " << id << "] begin read, value = " << g_value << std::endl;

            // 模拟读取耗时。
            std::this_thread::sleep_for(std::chrono::milliseconds(800));

            std::cout << "[Reader " << id << "] end read" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

// 写线程
void writer()
{
    for (int i = 0; i < 5; ++i)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        {
            std::unique_lock<std::shared_mutex> lock(g_mutex);

            std::cout << "========== Writer begin ==========" << std::endl;

            // 模拟写操作。
            ++g_value;

            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            std::cout << "Writer update value = " << g_value << std::endl;

            std::cout << "========== Writer end ==========" << std::endl;
        }
    }
}

int main()
{
    std::thread t1(reader, 1);
    std::thread t2(reader, 2);
    std::thread t3(writer);

    t1.join();
    t2.join();
    t3.join();
}
