// 加锁的懒汉式实现。
#include <mutex>


class Singleton2
{

public:

    // 获取单实例对象。
    static Singleton2 *GetInstance();

    // 释放单实例，进程退出时调用。
    static void DeleteInstance();

    // 打印实例地址。
    void print();


private:

    // 将其构造和析构成为私有的, 禁止外部构造和析构。
    Singleton2();

    ~Singleton2();

    // 将其拷贝构造和赋值构造成为私有函数, 禁止外部拷贝和赋值。
    Singleton2(const Singleton2 &single);

    const Singleton2 &operator=(const Singleton2 &single);


private:

    // 唯一单实例对象指针。类内静态成员变量。
    static Singleton2 *m_singleton2;

    static std::mutex m_mutex;
};
