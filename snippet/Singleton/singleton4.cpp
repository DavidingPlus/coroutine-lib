#include "singleton4.h"

#include <iostream>


// 饿汉式单例：在程序启动阶段就创建唯一实例。这里的静态成员变量初始化发生在 main() 函数执行之前，此时通常还没有创建业务线程。因此对象只会被创建一次，不存在多个线程同时创建实例的问题，天然保证了初始化过程的线程安全。
// 缺点：
// 1. 不管是否使用 Singleton4，程序启动时都会创建对象。
// 2. 如果对象创建成本较高，会增加程序启动时间。
Singleton4 *Singleton4::m_singleton4 = new (std::nothrow) Singleton4();


Singleton4 *Singleton4::GetInstance()
{
    return m_singleton4;
}

void Singleton4::DeleteInstance()
{
    if (m_singleton4)
    {
        delete m_singleton4;
        m_singleton4 = nullptr;
    }
}

void Singleton4::print()
{
    std::cout << "void Singleton4::print(): " << this << std::endl;
}

Singleton4::Singleton4()
{
    std::cout << "Singleton4::Singleton4()" << std::endl;
}

Singleton4::~Singleton4()
{
    std::cout << "Singleton4::~Singleton4()" << std::endl;
}
