/*
 * dynamic_cast
 *
 * 用途：
 *   用于具有继承关系的类之间进行安全的运行时类型转换，
 *   主要用于基类指针/引用向派生类指针/引用的转换（向下转型）。
 *
 * 常见使用场景：
 *   1. Base* -> Derived*
 *   2. Base& -> Derived&
 *   3. 多态体系中根据实际对象类型执行不同逻辑
 *
 * 特点：
 *   - 会进行运行时类型检查。
 *   - 需要基类至少包含一个虚函数（多态类型）。
 *   - 转换成功返回目标类型指针。
 *   - 转换失败：
 *       指针转换返回 nullptr；
 *       引用转换抛出 std::bad_cast。
 *   - 比 static_cast 更安全，但需要 RTTI 支持，并存在一定运行时开销。
 *
 * 与 static_cast 区别：
 *   - static_cast：
 *       编译期转换，不检查对象真实类型，
 *       错误的向下转型可能导致未定义行为。
 *
 *   - dynamic_cast：
 *       运行时检查对象真实类型，
 *       转换失败可以被检测。
 *
 * 适用原则：
 *   当基类指针实际可能指向不同派生类型，且需要根据真实类型进行处理时，使用 dynamic_cast。
 */
#include <gtest/gtest.h>


/*
 * 为什么 dynamic_cast 需要虚函数？
 *
 * dynamic_cast 需要依赖 RTTI（Run-Time Type Information，运行时类型信息）来判断对象在运行时的真实类型。
 *
 * 若一个类包含虚函数，编译器会为该类生成虚函数表（vtable），在对象内部保存一个指向虚函数表的指针（vptr）。vtable 中除了保存虚函数地址外，还包含该类型对应的 RTTI 信息。
 *
 * dynamic_cast 执行向下转型时，会通过对象内部的 vptr 找到对应的 RTTI 信息，然后检查当前对象的真实类型是否是目标类型，或者是否继承自目标类型。
 *
 * 转换过程：
 *
 *     Base*
 *       |
 *       | dynamic_cast<Derived*>
 *       ↓
 *
 *     通过对象内部 vptr
 *       |
 *       ↓
 *
 *     查询 RTTI 类型信息
 *       |
 *       ↓
 *
 *     判断真实对象类型
 *       |
 *       ├── 是 Derived 或其子类
 *       |       ↓
 *       |    返回 Derived*
 *       |
 *       └── 不是 Derived
 *               ↓
 *            返回 nullptr
 *
 *
 * 如果基类没有虚函数：
 *
 *     class Base {};
 *
 * 对象内部不会保存 vptr，也就没有 RTTI 信息，dynamic_cast 无法知道对象运行时的真实类型，因此无法进行安全的类型检查。
 *
 * 所以：
 *
 *     dynamic_cast
 *         =
 *     多态类型（虚函数）
 *         +
 *     RTTI
 *         +
 *     运行时类型检查
 */
class Base
{

public:

    virtual ~Base() = default;
};


class Derived : public Base
{

public:

    int value = 100;
};

class OtherDerived : public Base
{
};


TEST(DynamicCastTest, DownCastSuccess)
{
    Base *base = new Derived();

    Derived *derived = dynamic_cast<Derived *>(base);

    ASSERT_NE(derived, nullptr);

    EXPECT_EQ(derived->value, 100);

    delete base;
}

TEST(DynamicCastTest, DownCastFail)
{
    Base *base = new OtherDerived();

    Derived *derived = dynamic_cast<Derived *>(base);

    EXPECT_EQ(derived, nullptr);

    delete base;
}

TEST(DynamicCastTest, CompareWithStaticCast)
{
    Base *base = new OtherDerived();

    // static_cast：不检查真实类型，直接认为它是 Derived。
    Derived *d1 = static_cast<Derived *>(base);

    // dynamic_cast：检查 RTTI。
    Derived *d2 = dynamic_cast<Derived *>(base);

    // 只是地址转换成功。
    EXPECT_NE(d1, nullptr);

    // 类型检查失败。
    EXPECT_EQ(d2, nullptr);

    delete base;
}

TEST(DynamicCastTest, ReferenceSuccess)
{
    Derived derived;

    Base &base = derived;

    Derived &result = dynamic_cast<Derived &>(base);

    EXPECT_EQ(result.value, 100);
}

TEST(DynamicCastTest, ReferenceFail)
{
    OtherDerived other;

    Base &base = other;

    EXPECT_THROW(
        {
            dynamic_cast<Derived &>(base);
        },
        std::bad_cast);
}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);


    return RUN_ALL_TESTS();
}
