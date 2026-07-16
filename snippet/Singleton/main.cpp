// 单例模式可以分为懒汉式和饿汉式，两者之间的区别在于创建实例的时间不同。
// 懒汉式：系统运行中，实例并不存在，只有当需要使用该实例时，才会去创建并使用实例。这种方式要考虑线程安全。
// 饿汉式：系统一运行，就初始化创建实例，当需要时，直接调用即可。这种方式本身就线程安全，没有多线程的线程安全问题。

// 单例类的特点：
// 1. 构造函数和析构函数为私有类型，目的是禁止外部构造和析构。
// 2. 拷贝构造函数和赋值构造函数是私有类型，目的是禁止外部拷贝和赋值，确保实例的唯一性。
// 3. 类中有一个获取实例的静态方法，可以全局访问。

#include "singleton1.h"
#include "singleton2.h"
#include "singleton3.h"
#include "singleton4.h"

#include <iostream>


int main()
{
    std::cout << "======================" << std::endl;

    std::cout << "Test Singleton1" << std::endl;
    auto &singleton1A = Singleton1::GetInstance();
    auto &singleton1B = Singleton1::GetInstance();
    singleton1A.print();
    singleton1B.print();
    std::cout << "same instance: " << std::boolalpha << (&singleton1A == &singleton1B) << std::endl;

    std::cout << "======================" << std::endl;

    std::cout << "Test Singleton2" << std::endl;
    auto *singleton2A = Singleton2::GetInstance();
    auto *singleton2B = Singleton2::GetInstance();
    singleton2A->print();
    singleton2B->print();
    std::cout << "same instance: " << std::boolalpha << (singleton2A == singleton2B) << std::endl;

    std::cout << "======================" << std::endl;

    std::cout << "Test Singleton3" << std::endl;
    auto singleton3A = Singleton3::GetInstance();
    auto singleton3B = Singleton3::GetInstance();
    singleton3A->print();
    singleton3B->print();
    std::cout << "same instance: " << std::boolalpha << (singleton3A.get() == singleton3B.get()) << std::endl;

    std::cout << "======================" << std::endl;

    std::cout << "Test Singleton4" << std::endl;
    auto *singleton4A = Singleton4::GetInstance();
    auto *singleton4B = Singleton4::GetInstance();
    singleton4A->print();
    singleton4B->print();
    std::cout << "same instance: " << std::boolalpha << (singleton4A == singleton4B) << std::endl;

    std::cout << "======================" << std::endl;

    Singleton2::DeleteInstance();
    Singleton4::DeleteInstance();
}
