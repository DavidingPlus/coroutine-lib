#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>


// std::condition_variable 提供了两种 wait() 函数。当前线程调用 wait() 后将被阻塞(此时当前线程应该获得了锁（mutex），不妨设获得锁 lck)，直到另外某个线程调用 notify_* 唤醒了当前线程，该函数会自动调用 lck.unlock() 释放锁，使得其他被阻塞在锁竞争上的线程得以继续执行。
// 在第二种情况下（即设置了 Predicate），只有当 pred 条件为 false 时调用 wait() 才会阻塞当前线程，并且在收到其他线程的通知后只有当 pred 为 true 时才会被解除阻塞。

// std::condition_variable::wait_for()
// 与 std::condition_variable::wait() 类似，不过 wait_for 可以指定一个时间段，在当前线程收到通知或者指定的时间 rel_time 超时之前，该线程都会处于阻塞状态。而一旦超时或者收到了其他线程的通知，wait_for 返回，剩下的处理步骤和 wait() 类似。
// 另外，wait_for 的重载版本（predicte(2)）的最后一个参数 pred 表示 wait_for 的预测条件，同 wait()。只有当 pred 条件为 false 时调用 wait() 才会阻塞当前线程，并且在收到其他线程的通知后只有当 pred 为 true 时才会被解除阻塞。

// std::condition_variable::wait_until()
// 与 std::condition_variable::wait_for 类似，但是 wait_until 可以指定一个时间点，在当前线程收到通知或者指定的时间点 abs_time 超时之前，该线程都会处于阻塞状态。而一旦超时或者收到了其他线程的通知，wait_until 返回，剩下的处理步骤和 wait_until() 类似。
// 另外，wait_until 的重载版本（predicte(2)）的最后一个参数 pred 表示 wait_until 的预测条件，同 wait()。只有当 pred 条件为 false 时调用 wait() 才会阻塞当前线程，并且在收到其他线程的通知后只有当 pred 为 true 时才会被解除阻塞。

std::mutex mtx;
std::condition_variable cv;


int cargoId = 0;
bool shipmentAvailable()
{
    return 0 != cargoId;
}


// 消费者线程.
void consume(int n)
{
    for (int i = 0; i < n; ++i)
    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, shipmentAvailable);

        // 消费者每次取走一个货物，并把 cargoId 设置为 0（即没有货物）。
        std::cout << "consumer thread " << std::this_thread::get_id() << " cargoId " << cargoId << '\n';
        cargoId = 0;
    }
}


int main()
{
    std::thread consumerThread(consume, 10); // 消费者线程.

    // 主线程为生产者线程, 生产 10 个物品.
    for (int i = 0; i < 10; ++i)
    {
        // 当货物 id 不为 0，也就是当前存在货物的时候就让出控制权。
        while (shipmentAvailable()) std::this_thread::yield();

        std::unique_lock<std::mutex> lck(mtx);
        cargoId = i + 1;
        cv.notify_one();
    }

    consumerThread.join();


    return 0;
}
