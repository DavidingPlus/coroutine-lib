#include "singleton1.h"

#include <iostream>


Singleton1 &Singleton1::GetInstance()
{
    // 局部静态特性的方式实现单实例。静态局部变量只在当前函数内有效，其他函数无法访问。静态局部变量只在第一次被调用的时候初始化，也存储在静态存储区，生命周期从第一次被初始化起至程序结束止。
    // 1. 延迟初始化：局部静态对象不会在程序启动时创建，而是在第一次调用 GetInstance() 时进行初始化。如果程序从未调用 GetInstance()，Singleton1 对象也不会被创建。
    // 2. 生命周期：局部静态变量存储在静态存储区，生命周期从第一次初始化开始，直到程序结束时自动析构。
    // 3. 作用域：instance 只在当前函数内部可见，外部无法直接访问，只能通过 GetInstance() 获取。
    // 4. C++11 线程安全保证：当多个线程同时第一次执行到局部静态变量初始化时，只有一个线程会负责完成初始化。其他线程会等待初始化完成，然后获取已经创建好的同一个对象。例如：线程 A 创建 instance，调用 Singleton1 构造函数。线程 B 发现 instance 正在初始化，等待线程 A 完成。最终 Singleton1 构造函数只会执行一次。注意 C++11 保证的是 "单例对象初始化过程线程安全"，并不保证 Singleton1 内部成员变量访问线程安全。例如：instance.count++; 多个线程同时执行时，仍然可能发生数据竞争。如果单例内部存在共享数据，需要使用 mutex、atomic 等同步机制进行保护。

    // 注意：C++11 线程安全只保证局部静态变量的初始化，下面两种常见的静态变量是不保证的。

    // 1. 类内静态成员变量：static 成员变量属于类本身，而不是某个对象实例。它的生命周期与程序运行周期相同。
    // class Singleton
    // {
    // private:
    //
    //     static Singleton instance;
    // };
    //
    // Singleton Singleton::instance;
    //
    // 这里的 instance 通常在程序启动阶段进行初始化，而不是第一次调用函数时初始化。因此：C++11 不会提供函数局部静态变量那种初始化线程安全保证。

    // 2. 全局静态变量：static Singleton instance;
    // 该变量定义在函数外部，具有文件作用域，并且生命周期贯穿整个程序运行期间。它通常在程序启动阶段完成初始化，在 main() 执行之前就已经创建。因此：它不属于 C++11 "函数局部静态变量初始化线程安全"的保证范围。

    static Singleton1 Singleton1;


    return Singleton1;
}

void Singleton1::Print()
{
    std::cout << "Singleton1::Print(): " << this << std::endl;
}

Singleton1::Singleton1()
{
    std::cout << "Singleton1::Singleton1()" << std::endl;
}

Singleton1::~Singleton1()
{
    std::cout << "Singleton1::~Singleton1()" << std::endl;
}
