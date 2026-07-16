#include <iostream>
#include <type_traits>
#include <typeinfo>


int main()
{
    // remove_reference_t<T>：去除类型 T 上的引用。
    using A = std::remove_reference_t<int>;    // int -> int
    using B = std::remove_reference_t<int &>;  // int& -> int
    using C = std::remove_reference_t<int &&>; // int&& -> int

    // std::boolalpha 是 C++ 流输出的一个格式控制符，作用是让 bool 类型输出为 true / false，而不是 1 / 0。
    std::cout << std::boolalpha;

    // is_same_v<T1, T2>：判断两个类型是否相同。
    std::cout << std::is_same_v<A, int> << std::endl; // true
    std::cout << std::is_same_v<B, int> << std::endl; // true
    std::cout << std::is_same_v<C, int> << std::endl; // true

    // is_reference_v<T>：判断类型是否为引用类型。
    std::cout << std::is_reference_v<int> << std::endl;    // false
    std::cout << std::is_reference_v<int &> << std::endl;  // true
    std::cout << std::is_reference_v<int &&> << std::endl; // true

    std::cout << std::is_reference_v<A> << std::endl; // false
    std::cout << std::is_reference_v<B> << std::endl; // false
    std::cout << std::is_reference_v<C> << std::endl; // false
}
