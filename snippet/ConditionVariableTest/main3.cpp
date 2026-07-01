#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>


std::mutex mtx;
std::condition_variable cv;

int cargoId = 0;


void consumer()
{
    std::unique_lock<std::mutex> lck(mtx);
    while (0 == cargoId) cv.wait(lck);

    std::cout << "consumer thread " << std::this_thread::get_id() << " cargoId " << cargoId << '\n';
    cargoId = 0;
}

void producer(int id)
{
    std::unique_lock<std::mutex> lck(mtx);
    cargoId = id;
    // std::condition_variable::notify_one()：唤醒某个等待(wait)线程。如果当前没有等待线程，则该函数什么也不做，如果同时存在多个等待线程，则唤醒某个线程是不确定的(unspecified)。
    // std::condition_variable::notify_all()：唤醒所有的等待(wait)线程。如果当前没有等待线程，则该函数什么也不做。
    cv.notify_one();
}


int main()
{
    std::thread consumers[10], producers[10];

    for (int i = 0; i < 10; ++i)
    {
        consumers[i] = std::thread(consumer);
        producers[i] = std::thread(producer, i + 1);
    }

    for (int i = 0; i < 10; ++i)
    {
        producers[i].join();
        consumers[i].join();
    }


    return 0;
}
