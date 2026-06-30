#include <iostream>
#include <memory>


class Widget : public std::enable_shared_from_this<Widget>
{
public:

    Widget()
    {
        std::cout << "Widget constructor run" << std::endl;
    }

    ~Widget()
    {
        std::cout << "Widget destructor run" << std::endl;
    }

    std::shared_ptr<Widget> GetSharedObject()
    {
        return shared_from_this();
    }
};


int main()
{
    // enable_shared_from_this<T> 就是为了解决这个问题。它内部含有 weak_ptr，是一个“不拥有对象”的指针，设置的初衷是为了解决 shard_ptr 相互引用时的死锁问题。它可以观察 shared_ptr 管理的对象，但是不会增加引用计数。在下面的 shared_from_this 函数中，shard_ptr p 的引用计数是 1，内部会把 weak_ptr 提升为 shard_ptr，weak_ptr 不修改引用计数，所有过程都没有再使用 shared_ptr 的普通构造函数，没有在产生额外的引用计数对象，不会存在把一个内存资源，进行多次计数的过程。
    // https://zhuanlan.zhihu.com/p/365414133
    // https://www.cnblogs.com/S1mpleBug/p/16770731.html
    std::shared_ptr<Widget> p = std::make_shared<Widget>(); // 等价于 std::shared_ptr<Widget> p(new Widget())。
    std::shared_ptr<Widget> q = p->GetSharedObject();

    std::cout << p.use_count() << std::endl;
    std::cout << q.use_count() << std::endl;


    return 0;
}
