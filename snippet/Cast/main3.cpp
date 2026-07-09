/*
 * reinterpret_cast
 *
 * 用途：
 *   在没有类型关系的对象之间重新解释（reinterpret）内存表示，
 *   不会进行任何数据转换，也不会进行运行时类型检查。
 *
 * 常见使用场景：
 *   1. 不同类型指针之间的转换（T1* <-> T2*）
 *   2. 指针与整数之间的转换（T* <-> uintptr_t）
 *   3. 函数指针之间的转换
 *   4. 查看对象的底层字节表示
 *   5. 底层开发（操作系统、驱动、网络协议、内存映射、硬件寄存器等）
 *
 * 特点：
 *   - 仅改变编译器解释内存的方式，不会修改内存中的实际数据。
 *   - 不检查两个类型之间是否存在任何关系。
 *   - 不进行运行时类型检查。
 *   - 几乎可以转换任意指针类型，因此风险最高。
 *   - 错误地访问转换后的对象通常会导致未定义行为（UB）。
 *
 * 适用原则：
 *   仅用于底层开发或确实需要按另一种类型解释同一块内存时使用，
 *   普通业务代码应尽量避免使用 reinterpret_cast。
 */
#include <gtest/gtest.h>


TEST(ReinterpretCastTest, PointerCast)
{
    int value = 100;

    int *p = &value;

    char *c = reinterpret_cast<char *>(p);

    // reinterpret_cast 不改变内存，只改变解释方式。
    EXPECT_EQ(reinterpret_cast<void *>(p), reinterpret_cast<void *>(c));
}

TEST(ReinterpretCastTest, AccessBytes)
{
    int value = 0x12345678;

    unsigned char *bytes = reinterpret_cast<unsigned char *>(&value);

    // 读取 int 对象的第一个字节。
    unsigned char first = bytes[0];

    // 这里只验证可以访问，不判断具体值。因为大小端不同，结果可能不同。
    EXPECT_TRUE(first >= 0);
}

TEST(ReinterpretCastTest, PointerToInteger)
{
    int value = 10;

    uintptr_t address = reinterpret_cast<uintptr_t>(&value);

    EXPECT_NE(address, 0);
}

TEST(ReinterpretCastTest, IntegerToPointer)
{
    int value = 100;

    uintptr_t address = reinterpret_cast<uintptr_t>(&value);

    int *p = reinterpret_cast<int *>(address);

    EXPECT_EQ(*p, 100);
}


struct A
{
    int value;
};


struct B
{
    int value;
};


TEST(ReinterpretCastTest, StructPointerCast)
{
    A a{123};

    B *b = reinterpret_cast<B *>(&a);

    // 这依赖两个结构体内存布局一致。在实际工程中非常危险，不推荐这么使用。
    EXPECT_EQ(b->value, 123);
}

TEST(ReinterpretCastTest, DifferenceWithStaticCast)
{
    double value = 3.14;

    // static_cast：真正转换数值。
    int a = static_cast<int>(value);

    EXPECT_EQ(a, 3);

    // reinterpret_cast：不能直接 double -> int，需要通过指针重新解释。
    int *p = reinterpret_cast<int *>(&value);

    // p 指向 double 的内存，*p 不是 3，读取它属于未定义行为，不推荐解引用。
    EXPECT_NE(p, nullptr);
}


void func() {}


TEST(ReinterpretCastTest, FunctionPointer)
{
    using FuncPtr = void (*)();

    FuncPtr p = &func;

    void *address = reinterpret_cast<void *>(p);

    EXPECT_NE(address, nullptr);
}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);


    return RUN_ALL_TESTS();
}
