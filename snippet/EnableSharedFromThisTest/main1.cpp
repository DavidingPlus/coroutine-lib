#include <iostream>
#include <memory>


class Widget
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
        return std::shared_ptr<Widget>(this);
    }
};


int main()
{
    // 这两行代码会创建两个不同的 shared_ptr 对象，内部指向同一个 Widget 对象，但这两个 shard_ptr 对象是独立的，必须的引用计数是分开的，这里都是 1。因为在析构的时候就会触发两次，程序会崩溃。
    std::shared_ptr<Widget> p(new Widget());
    std::shared_ptr<Widget> q = p->GetSharedObject();

    std::cout << p.use_count() << std::endl;
    std::cout << q.use_count() << std::endl;


    return 0;
}
