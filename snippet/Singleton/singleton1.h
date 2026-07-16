// 内部静态变量的懒汉实现。
class Singleton1
{

public:

    // 获取单实例对象。
    static Singleton1 &GetInstance();

    // 打印实例地址。
    void print();


private:

    // 禁止外部构造。
    Singleton1();

    // 禁止外部析构。
    ~Singleton1();

    // 禁止外部拷贝构造。
    Singleton1(const Singleton1 &Singleton1) = delete;

    // 禁止外部赋值操作。
    Singleton1 &operator=(const Singleton1 &Singleton1) = delete;
};
