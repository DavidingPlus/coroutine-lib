#include <iostream>
#include <queue>


int main()
{
    /*
     * std::priority_queue 是基于堆实现的优先队列。std::priority_queue 允许你以 O(1) 的时间复杂度访问队列中的最大元素，并且插入和删除操作的时间复杂度为 O(log n)。
     * 注意：既然是基于堆实现的，只保证堆顶元素是最高优先级元素（最大值或最小值），并不保证整个容器中的元素严格有序。
     *
     * 模板参数：
     * 1. 第一个参数：元素类型。
     * 2. 第二个参数：底层存储容器，默认是 std::vector。
     * 3. 第三个参数：比较器，决定堆的类型。
     *
     * 默认使用 std::less<T>，形成最大堆；
     * 使用 std::greater<T> 可以形成最小堆。
     *
     * 例如：
     * priority_queue<int, vector<int>, greater<int>>
     * 表示使用 vector 存储 int，并构造一个最小堆。
     */
    std::priority_queue<int> pq;

    pq.push(10);
    pq.push(5);
    pq.push(8);
    pq.push(3);
    pq.push(7);

    std::cout << "priority_queue: ";

    while (!pq.empty())
    {
        std::cout << pq.top() << " ";

        pq.pop();
    }

    std::cout << std::endl;
}
