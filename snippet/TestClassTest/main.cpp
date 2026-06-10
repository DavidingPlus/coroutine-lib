#include <iostream>

#include "ltest.h"


int main()
{
    std::cout << LTest::foo() << std::endl; // hello world

    auto p = LTest().gee(3, 4);

    std::cout << p.first << std::endl;  // 3
    std::cout << p.second << std::endl; // 4


    return 0;
}
