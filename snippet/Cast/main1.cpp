/*
 * static_cast
 *
 * 用途：
 *   编译期类型转换，用于具有明确类型关系且编译器能够检查合法性的转换。
 *
 * 常见使用场景：
 *   1. 内置数值类型转换
 *      - double -> int
 *      - int -> double
 *      - float -> long
 *      - char <-> int
 *      - bool <-> int
 *
 *   2. 枚举类型转换
 *      - enum -> int
 *      - int -> enum（不检查枚举值是否合法）
 *
 *   3. void* 与具体类型指针转换
 *      - void* -> T*
 *      - T* -> void*
 *
 *   4. 继承体系中的类型转换
 *      - Derived* -> Base*（向上转型，安全）
 *      - Base* -> Derived*（向下转型，不检查对象真实类型，错误使用会导致未定义行为。使用 dynamic_cast 做运行期类型检查来保证向下转型的正确性）
 *      - 引用(Base& / Derived&)同理。
 *
 * 特点：
 *   - 编译期完成，不进行运行时类型检查。
 *   - 不改变转换规则，只是将隐式转换显式化，提高代码可读性。
 *   - 无法移除 const/volatile 属性（应使用 const_cast）。
 *   - 无法转换两个毫无关系的类型（如 double* -> int*）。
 *
 * 适用原则：
 *   只要两个类型之间本身存在合法的转换关系，并且不需要运行时检查，
 *   就应该优先使用 static_cast。
 */
#include <gtest/gtest.h>


TEST(StaticCastTest, DoubleToInt)
{
    double value = 3.14;

    int number = static_cast<int>(value);

    EXPECT_EQ(number, 3);
}

TEST(StaticCastTest, IntToDouble)
{
    int value = 10;

    double number = static_cast<double>(value);

    EXPECT_DOUBLE_EQ(number, 10.0);
}

TEST(StaticCastTest, BoolToInt)
{
    bool b = true;

    int value = static_cast<int>(b);

    EXPECT_EQ(value, 1);
}

TEST(StaticCastTest, IntToBool)
{
    bool a = static_cast<bool>(0);
    bool b = static_cast<bool>(100);

    EXPECT_FALSE(a);
    EXPECT_TRUE(b);
}

TEST(StaticCastTest, CharToInt)
{
    char c = 'A'; // 65

    int value = static_cast<int>(c);

    EXPECT_EQ(value, 65);
}

TEST(StaticCastTest, IntToChar)
{
    int value = 66; //'B'

    char c = static_cast<char>(value);

    EXPECT_EQ(c, 'B');
}


enum class Color
{
    Red = 1,
    Green = 2,
    Blue = 3
};


TEST(StaticCastTest, EnumToInt)
{
    EXPECT_EQ(static_cast<int>(Color::Green), 2);
}

TEST(StaticCastTest, IntToEnum)
{
    Color color = static_cast<Color>(3);

    EXPECT_EQ(Color::Blue, color);
}

TEST(StaticCastTest, VoidPointer)
{
    int value = 100;

    void *ptr = &value;

    int *p = static_cast<int *>(ptr);

    EXPECT_EQ(*p, 100);
}


class Base
{

public:

    int base = 10;
};

class Derived : public Base
{

public:

    int child = 20;
};

TEST(StaticCastTest, UpCast)
{
    Derived d;

    Base *p = static_cast<Base *>(&d);

    EXPECT_EQ(p->base, 10);
}

TEST(StaticCastTest, DownCast)
{
    // 如果这里 d 是基类的话，下面的 child 代码，编译仍然能通过，因为编译器只知道 Base 和 Derived 有继承关系，但是他无法知道运行时 child 究竟指向谁，他不会做运行时检查，会直接转换。如果这样做的话就会访问不存在的内存发生未定义行为。
    // dynamic_cast 会在运行时借助 RTTI 检查 child 对象究竟是不是 Derived，因此更加安全。
    Derived d;

    Base *base = &d;

    Derived *child = static_cast<Derived *>(base);

    EXPECT_EQ(child->child, 20);
}

TEST(StaticCastTest, ImplicitCast)
{
    double value = 8.88;

    int a = value;

    int b = static_cast<int>(value);

    EXPECT_EQ(a, b);
}

TEST(StaticCastTest, Nullptr)
{
    int *p = static_cast<int *>(nullptr);

    EXPECT_EQ(p, nullptr);
}

TEST(StaticCastTest, ReferenceCast)
{
    double value = 8.5;

    int res = static_cast<int>(value);

    const int &ref = res;

    EXPECT_EQ(ref, 8);
}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);


    return RUN_ALL_TESTS();
}
