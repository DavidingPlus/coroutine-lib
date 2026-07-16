#include "singleton2.h"

#include <iostream>


// 初始化静态成员变量。
Singleton2 *Singleton2::m_singleton2 = nullptr;

std::mutex Singleton2::m_mutex;


// 注意：不能返回指针的引用（Singleton2 * &），否则存在外部被修改的风险！
// Singleton2 *&ptr = Singleton2::GetInstance();
// ptr = nullptr; 这样指向全局唯一单例对象的指针对象 m_singleton2 就被破坏了。
// 返回指针对象，外部是修改不了原本的那个 m_singleton2 指针的。
Singleton2 *Singleton2::GetInstance()
{
    // 这里使用了两个 if 判断语句的技术称为双检锁；好处是，只有判断指针为空的时候才加锁，避免每次调用 GetInstance 的方法都加锁，锁的开销毕竟还是有点大的。
    // 第一次检查：如果单例对象已经创建，则直接返回对象指针。这里不加锁是为了提高性能，避免每次调用 GetInstance() 都进行加锁和解锁操作。因为单例创建完成以后，后续大量调用只需要读取指针即可。
    if (nullptr == m_singleton2)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        // 第二次检查：因为多个线程可能同时通过第一次检查，但是只有一个线程能够获得锁。
        //
        // 例如：
        //
        // 线程 A:
        //     第一次检查发现为空
        //     获得锁
        //     创建对象
        //
        // 线程 B:
        //     第一次检查发现为空
        //     等待锁
        //
        // 当线程 B 获得锁时，线程 A 可能已经完成对象创建。所以需要再次判断，避免重复创建多个单例对象。
        if (nullptr == m_singleton2)
        {
            // volatile 防止编译器对创建对象过程进行过度优化，避免单例指针提前暴露给其他线程。实际上 volatile 不能保证多线程安全，不能替代 mutex 或 atomic。
            volatile auto temp = new (std::nothrow) Singleton2();
            m_singleton2 = temp;
        }
    }


    return m_singleton2;
}

void Singleton2::DeleteInstance()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_singleton2)
    {
        delete m_singleton2;
        m_singleton2 = nullptr;
    }
}

void Singleton2::print()
{
    std::cout << "void Singleton2::print(): " << this << std::endl;
}

Singleton2::Singleton2()
{
    std::cout << "Singleton2::Singleton2()" << std::endl;
}

Singleton2::~Singleton2()
{
    std::cout << "Singleton2::~Singleton2()" << std::endl;
}
