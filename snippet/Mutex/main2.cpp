#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <list>
#include <mutex>


// 为了解决 lock_guard 锁的粒度过大的原因，unique_lock 就出现了。
// unique_lock 会在构造函数加锁，然后可以利用 unique.unlock() 来解锁，所以当你觉得锁的粒度太多的时候，可以利用这个来解锁，而析构的时候会判断当前锁的状态来决定是否解锁。如果当前状态已经是解锁状态了，那么就不会再次解锁，而如果当前状态是加锁状态，就会自动调用 unique.unlock() 来解锁。而 lock_guard 在析构的时候一定会解锁，也没有中途解锁的功能。
// 当然，方便肯定是有代价的，unique_lock 内部会维护一个锁的状态，所以在效率上肯定会比 lock_guard 慢。

// unique_lock 的成员函数
// lock()：加锁。
// unlock()：解锁。
// try_lock()：尝试给互斥量加锁，如果拿不到锁，返回 false。如果拿到了锁，返回 true，这个函数是不阻塞的。
// release()：返回它所管理的 mutex 对象指针，并释放所有权；也就是说，这个 unique_lock 和 mutex 不再有关系。严格区分 unlock() 与 release() 的区别，不要混淆。如果原来 mutex 对象处于加锁状态，你有责任接管过来并负责解锁。（release 返回的是原始 mutex 的指针）。因此 unique_lock 对象这个 mutex 的所有权是可以转移，但是不能复制。
// 这句合法：std::unique_lock<std::mutex> sbguard2(std::move(sbguard)); // 移动语义，现在先当与 sbguard2 与 mutex 绑定到一起了。现在 sbguard1 指向空，sbguard2 指向了 mutex。

class A
{
public:

    void inMsgRecvQueue()
    {
        for (int i = 0; i < 10000; i++)
        {
            std::cout << "inMsgRecvQueue()执行，插入一个元素" << i << std::endl;
            {
                // std::unique_lock<std::mutex> 的构造函数，第一个参数是锁，与 std::lock_guard<std::mutex> 不同的是，他可以携带第二个参数，起一个标记作用。
                // std::adopt_lock：表示这个互斥量已经被 lock 了。你必须要把互斥量提前 lock 了，否则会报异常。
                // std::try_to_lock：尝试用 mutex 的 lock() 去锁定这个 mutex，但如果没有锁定成功，我也会立即返回，并不会阻塞在那里。前提是你自己不能先 lock。
                // std::defer_lock：用 std::defer_lock 的前提是，你不能自己先 lock，否则会报异常。std::defer_lock 的意思就是并没有给 mutex 加锁：初始化了一个没有加锁的 mutex。
                std::unique_lock<std::mutex> sbguard(mutex, std::try_to_lock);
                if (sbguard.owns_lock())
                {
                    // 拿到了锁
                    msgRecvQueue.push_back(i);
                    //...
                    // 其他处理代码
                }
                else
                {
                    // 没拿到锁
                    std::cout << "inMsgRecvQueue()执行，但没拿到锁头，只能干点别的事" << i << std::endl;
                }
            }
        }
    }

    bool outMsgLULProc(int &command)
    {
        mutex.lock(); // 要先 lock(),后续才能用 unique_lock 的 std::adopt_lock 参数。
        std::unique_lock<std::mutex> sbguard(mutex, std::adopt_lock);

        std::chrono::milliseconds dura(20000);
        std::this_thread::sleep_for(dura); // 休息 20s。

        if (!msgRecvQueue.empty())
        {
            // 消息不为空。
            int command = msgRecvQueue.front(); // 返回第一个元素，但不检查元素是否存在。
            msgRecvQueue.pop_front();           // 移除第一个元素。但不返回；


            return true;
        }


        return false;
    }

    // 把数据从消息队列取出的线程。
    void outMsgRecvQueue()
    {
        int command = 0;
        for (int i = 0; i < 10000; i++)
        {
            bool result = outMsgLULProc(command);

            if (result == true)
            {
                std::cout << "outMsgRecvQueue()执行，取出一个元素" << std::endl;
                // 处理数据
            }
            else
            {
                // 消息队列为空
                std::cout << "inMsgRecvQueue()执行，但目前消息队列中为空！" << i << std::endl;
            }
        }
        std::cout << "end!" << std::endl;
    }

private:

    std::list<int> msgRecvQueue; // 容器（消息队列），代表玩家发送过来的命令。
    std::mutex mutex;            // 创建一个互斥量（一把锁）。
};

int main()
{
    A myobja;

    std::thread myOutMsgObj(&A::outMsgRecvQueue, &myobja);
    std::thread myInMsgObj(&A::inMsgRecvQueue, &myobja);

    myOutMsgObj.join();
    myInMsgObj.join();

    std::cout << "主线程执行！" << std::endl;


    return 0;
}
