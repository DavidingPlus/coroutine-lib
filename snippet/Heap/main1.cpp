#include <iostream>
#include <vector>
#include <algorithm>


void printHeap(const std::vector<int> &vec)
{
    std::cout << "heap: ";
    for (auto n : vec) std::cout << n << " ";
    std::cout << std::endl;
}


int main()
{
    std::vector<int> nums = {3, 1, 5, 7, 2, 8, 4};

    // 建堆，默认建立最大堆。使用的谓词是 std::less<T> 或者 std::less_equal<T>。
    // 最小堆使用谓词 std::greater<T> 或者 std::greater_int<T>。
    // make_heap() 这类函数的接口接收的参数是随机访问容器的迭代器，因此上面的初始容器这里使用 vector。
    std::make_heap(nums.begin(), nums.end(), std::less_equal<int>());

    printHeap(nums);

    // 向堆中插入元素。先向 vector 中插入，然后使用 push_heap() 恢复堆结构。
    nums.push_back(10);
    std::push_heap(nums.begin(), nums.end());

    printHeap(nums);

    // 删除堆顶元素。pop_heap() 只是将堆顶元素和末尾元素交换，并调整剩下结构，并没有删除堆顶元素。因此还需要调用容器的 pop_back()。
    std::pop_heap(nums.begin(), nums.end());
    nums.pop_back();

    printHeap(nums);
}
