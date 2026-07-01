#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>


// std::cv_status 枚举类型
// cv_status::no_timeout：wait_for 或者 wait_until 没有超时，即在规定的时间段内线程收到了通知。
// cv_status::timeout：wait_for 或者 wait_until 超时。

std::mutex mtx;
std::condition_variable cv;
bool ready = false;


void printId(int id)
{
    std::unique_lock<std::mutex> lck(mtx);
    while (!ready) cv.wait(lck);
    // ...
    std::cout << "thread " << id << '\n';
}

void go()
{
    std::unique_lock<std::mutex> lck(mtx);
    // 当调用该函数的线程退出时，所有在 cond 条件变量上等待的线程都会收到通知。
    std::notify_all_at_thread_exit(cv, std::move(lck));
    ready = true;
}


int main()
{
    std::thread threads[10];
    for (int i = 0; i < 10; ++i) threads[i] = std::thread(printId, i);
    std::cout << "10 threads ready to race...\n";

    std::thread(go).detach(); // go!

    for (auto &th : threads) th.join();


    return 0;
}
