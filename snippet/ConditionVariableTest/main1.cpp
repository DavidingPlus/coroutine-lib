#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>


// 当 std::condition_variable 对象的某个 wait 函数被调用的时候，它使用 std::unique_lock(通过 std::mutex) 来锁住当前线程。当前线程会一直被阻塞，直到另外一个线程在相同的 std::condition_variable 对象上调用了 notification 函数来唤醒当前线程。
// std::condition_variable 对象通常使用 std::unique_lock<std::mutex> 来等待，如果需要使用另外的 lockable 类型，可以使用 std::condition_variable_any 类。

std::mutex mtx;             // 全局互斥锁。
std::condition_variable cv; // 全局条件变量。
bool ready = false;         // 全局标志位。


void printId(int id)
{
    std::unique_lock<std::mutex> lck(mtx);

    // 如果标志位不为 true，则等待...
    while (!ready) cv.wait(lck); // 当前线程被阻塞，当全局标志位变为 true 之后，线程被唤醒，继续往下执行打印线程编号 id。
    std::cout << "thread " << id << '\n';
}

void notifyAllThread()
{
    std::unique_lock<std::mutex> lck(mtx);
    ready = true;    // 设置全局标志位为 true。
    cv.notify_all(); // 唤醒所有线程。
}


int main()
{
    std::thread threads[10];
    for (int i = 0; i < 10; ++i) threads[i] = std::thread(printId, i);

    std::cout << "10 threads ready to race...\n";
    notifyAllThread();

    for (auto &th : threads) th.join();


    return 0;
}
