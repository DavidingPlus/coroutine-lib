/*
 * const_cast
 *
 * 用途：
 *   修改对象的 const 或 volatile 属性。
 *
 * 常见使用场景：
 *   1. const T* <-> T*
 *   2. const T& <-> T&
 *   3. this 指针去除 const（较少使用）
 *
 * 特点：
 *   - 只能修改 const/volatile 限定符，不能改变对象类型。
 *   - 如果对象本身不是 const，只是通过 const 指针/引用访问，去除 const 后修改对象是合法的。
 *   - 如果对象本身就是 const，去除 const 后再修改，属于未定义行为（UB）。
 *
 * 适用原则：
 *   仅在对象原本不是 const，但接口要求 const 或历史原因导致 const 不匹配时使用。
 */
#include <gtest/gtest.h>


TEST(ConstCastTest, RemoveConstPointer)
{
    int value = 100;

    // const int* a：const 与 int 结合，因此变量 a 是一个指向常量整型的指针（整型不能改）。
    // int* const a：const 与 * 结合，因此变量 a 是一个指向整型的常量指针（指针不能改）。
    const int *p = &value;

    int *q = const_cast<int *>(p);

    *q = 200;

    EXPECT_EQ(value, 200);
}

TEST(ConstCastTest, RemoveConstReference)
{
    int value = 10;

    const int &ref = value;

    int &r = const_cast<int &>(ref);

    r = 30;

    EXPECT_EQ(value, 30);
}


void Change(int *p)
{
    *p = 999;
}

TEST(ConstCastTest, FunctionArgument)
{
    int value = 1;

    const int *p = &value;

    Change(const_cast<int *>(p));

    EXPECT_EQ(value, 999);
}


class Cache
{
public:

    // int Get() const
    // {
    //     auto self = const_cast<Cache *>(this);

    //     self->m_cache = 100;

    //     return self->m_cache;
    // }

    int Get() const
    {
        m_cache = 100;

        return m_cache;
    }


private:

    // mutable 用于修饰类的数据成员，表示该成员不受对象 const 属性限制，即使对象是 const 或在 const 成员函数中也允许修改。它通常用于缓存、延迟初始化、统计信息和互斥锁等不影响对象逻辑状态的成员，使 const 成员函数能够修改这些实现细节而不破坏其语义。
    mutable int m_cache = 0;
};

TEST(ConstCastTest, ThisPointer)
{
    Cache cache;

    EXPECT_EQ(cache.Get(), 100);
}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);


    return RUN_ALL_TESTS();
}
