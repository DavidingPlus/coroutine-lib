#include "singleton3.h"


static std::shared_ptr<Singleton3> singleton3 = nullptr;

// std::once_flag 用于配合 std::call_once 使用。once_flag 内部保存一个状态，用来记录初始化函数是否已经成功执行过。多个线程必须共享同一个 once_flag，才能保证同一段初始化代码只执行一次。
static std::once_flag singleton3Flag;


std::shared_ptr<Singleton3> Singleton3::GetInstance()
{
    // std::call_once 保证传入的初始化函数只执行一次。多个线程同时调用 GetInstance() 时，call_once 会根据 once_flag 的状态决定行为：
    // 1. 如果初始化已经完成：直接返回，不会再次执行初始化函数。
    // 2. 如果多个线程同时第一次调用：只有一个线程能够执行初始化函数，其他线程会等待该线程完成初始化。
    // 3. 初始化完成后：once_flag 会记录完成状态，后续线程调用 call_once 时会直接跳过初始化函数。
    // 4. 如果初始化函数执行失败（例如抛异常）：本次初始化不会被认为成功，下次调用 call_once 时仍然会重新尝试初始化。
    std::call_once(singleton3Flag, [&]
                   { singleton3 = std::shared_ptr<Singleton3>(new Singleton3()); });


    return singleton3;
}

void Singleton3::print()
{
    std::cout << "Singleton3::print(): " << this << std::endl;
}

Singleton3::Singleton3()
{
    std::cout << "Singleton3::Singleton3()" << std::endl;
}

Singleton3::~Singleton3()
{
    std::cout << "Singleton3::~Singleton3()" << std::endl;
}
