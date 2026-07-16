// 使用 C++11 std::call_once 实现的懒汉单例，C++11 线程安全。
#include <iostream>
#include <memory>
#include <mutex>


class Singleton3
{

public:

    static std::shared_ptr<Singleton3> GetInstance();

    void print();


private:

    Singleton3();

    ~Singleton3();

    static void DeleteInstance(Singleton3 *singleton3);

    Singleton3(const Singleton3 &Singleton3) = delete;

    Singleton3 &operator=(const Singleton3 &Singleton3) = delete;
};
