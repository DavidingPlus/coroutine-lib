#include <iostream>


// std::forward 是 C++11 中引入的一个函数模板，用于实现完美转发。它的作用是根据传入的参数，决定将参数以左值引用还是右值引用的方式进行转发。
// 传统上，当一个左值传递给一个函数时，参数会以左值引用的方式进行传递；当一个右值传递给一个函数时，参数会以右值引用的方式进行传递。完美转发是为了解决传递参数时的临时对象（右值）被强制转换为左值的问题。std::forward 实现完美转发主要用于以下场景：提高模板函数参数传递过程的转发效率。


template <typename T>
void print(T &t)
{
    std::cout << "Lvalue ref" << std::endl;
}

// 对于形如 T&& 的变量或者参数，如果 T 可以进行推导，那么 T&& 称之为万能引用。对于形如 T&& 的类型来说，其既可以绑定左值，又可以绑定右值，而这个的前提是 T 需要进行推导。
// 1. 存在类型推导，param 是一个万能引用。
// template <typename T>
// void f(T &&param);
// 2. 存在类型推导，var 是一个万能引用。
// auto &&var = var1;
// 3. 只有当发生自动类型推断时（例如：函数模板的类型自动推导），T&& 才是万能引用。下面一个示例中的 && 并不是一个万能引用。
// template <typename T>
// class Test
// {
//     Test(Test &&rhs); // Test 是一个特定的类型，不需要类型推导，所以 && 表示右值引用。
// };

template <typename T>
void print(T &&t)
{
    std::cout << "Rvalue ref" << std::endl;
}

/*
 * 引用折叠：当模板推导过程中出现引用嵌套时，C++ 会自动将多个引用合并成一个引用。
 *
 * 规则：
 *
 * T&  &  -> T&
 * T&  && -> T&
 * T&& &  -> T&
 * T&& && -> T&&
 *
 * 只要其中有一个左值引用 (&)，最终就是左值引用。
 * 只有两个右值引用 (&&) 合并，才保持右值引用。
 *
 * 示例：
 *
 * int x;
 *
 * func(x);
 *
 * T = int&
 *
 * T&&
 * = int& &&
 * = int&        // 引用折叠
 *
 *
 * func(10);
 *
 * T = int
 *
 * T&&
 * = int&&       // 保持右值引用
 */
// T&& 是万能引用。当传入不同类型的参数时，T 会发生推导，可能产生引用嵌套。C++ 会通过引用折叠规则，将嵌套引用转换为最终类型。
template <typename T>
void testForward(T &&v)
{
    // 注意：即使 v 的类型是 T&&（可能是右值引用），但是在 print(v) 语句中，v 是一个有名字的变量。C++ 规定：所有具名变量都是左值。所以这里的 v 永远是左值。
    print(v);
    // std::move 会无条件将 v 转换为右值引用，所以这里永远调用右值版本。
    print(std::move(v));
    // std::forward 会根据 T 保留参数最初的值类别：传入左值 -> 保持左值；传入右值 -> 恢复右值。这就是完美转发。
    print(std::forward<T>(v));

    std::cout << "======================" << std::endl;
}


int main(int argc, char *argv[])
{
    int x = 1;
    testForward(x);            // 实参为左值。
    testForward(std::move(x)); // 实参为右值。
}
