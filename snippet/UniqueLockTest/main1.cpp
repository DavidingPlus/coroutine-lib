#include <iostream>
#include <thread>
#include <string>
#include <mutex>
#include <chrono>


std::mutex mt;


void thread_task()
{
    for (int i = 0; i < 10; i++)
    {
        {
            std::lock_guard<std::mutex> guard(mt);
            std::cout << "child thread: " << i << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main()
{
    std::thread t(thread_task);

    for (int i = 0; i > -10; i--)
    {
        // lock_guard 在构造函数里加锁，在析构函数里解锁。C++ 委员会的解释是防止使用 mutex 加锁解锁的时候，忘记解锁 unlock 了。
        // 调用析构函数是大括号 {} 结束的时候，也就是说定义 lock_guard 的时候调用构造函数加锁，大括号解锁的时候调用析构函数解锁。
        // 这就产生了一个问题，如果这个定义域范围很大的话，那么锁的粒度就很大，很大程序上会影响效率。
        // 一种解决办法是为 std::lock_guard<std::mutex> 手动包装一个作用域，只管理想控制的部分，出了这个作用域就析构释放。
        {
            std::lock_guard<std::mutex> guard(mt);
            std::cout << "main thread: " << i << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    t.join();


    return 0;
}
