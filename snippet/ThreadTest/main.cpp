#include <iostream>
#include <memory>
#include <vector>
#include <chrono>

#include "thread.h"


void func()
{
    std::cout << "id: " << Thread::GetThreadId() << ", name: " << Thread::GetName();
    std::cout << ", this id: " << Thread::GetThis()->getId() << ", this name: " << Thread::GetThis()->getName() << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(3));
}


int main()
{
    std::vector<std::shared_ptr<Thread>> thrs;

    for (int i = 0; i < 5; ++i)
    {
        std::shared_ptr<Thread> thr = std::make_shared<Thread>(&func, "thread_" + std::to_string(i));

        thrs.push_back(thr);
    }

    for (int i = 0; i < 5; ++i)
    {
        thrs[i]->join();
    }


    return 0;
}
