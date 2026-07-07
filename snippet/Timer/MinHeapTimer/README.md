# 最小堆定时器（MinHeap Timer）

基于**最小堆（Min Heap）**实现的轻量级定时器管理器。

该实现支持：

- 添加定时任务
- 自动执行到期任务
- 根据 TimerId 取消任务
- 高效维护大量定时器

核心数据结构：

- `vector`：模拟完全二叉树，实现最小堆
- `unordered_map`：维护 TimerId 到堆节点位置的映射，实现 O(1) 查找删除

## 一、设计思想

传统定时器如果使用普通数组存储：

```
Timer1
Timer2
Timer3
...
```

每次寻找最近到期任务需要遍历全部 Timer：

```
O(n)
```

当定时器数量增加时，每次 tick 都需要扫描整个数组，效率较低。


本实现使用最小堆：

```
             Timer(10ms)
              /       \
        Timer(50ms)  Timer(100ms)
          /
    Timer(200ms)
```

保证：

```
父节点 expire <= 子节点 expire
```

因此：

- 堆顶永远是最早到期的 Timer
- 每次 tick 只需要检查堆顶

时间复杂度：

| 操作 | 复杂度 |
|-|-|
| 添加 Timer | O(logN) |
| 删除 Timer | O(logN) |
| 获取最近 Timer | O(1) |
| 执行到期 Timer | O(klogN) |


其中：

- N：当前 Timer 数量
- k：本次 tick 到期的 Timer 数量

# 二、数据结构


## 1. Timer


```cpp
class Timer
{
    uint64_t m_expire;

    Callback m_callback;

    uint64_t m_id;
};
```


每个 Timer 保存：

### expire

定时器触发时间。

单位：

```
milliseconds
```


例如：

```cpp
now + 1000
```

表示：

```
1秒后执行
```


---

### callback

任务到期后的执行函数。


例如：

```cpp
tmr.add(
    now + 1000,
    []()
    {
        printf("timeout\n");
    });
```


---

### id

Timer 唯一编号。

用于：

```
TimerId -> heap index
```

快速定位 Timer。

# 三、为什么需要 TimerId


如果只有堆：

```
vector<Timer> m_heap;
```

删除指定 Timer：

```
cancel(timer)
```

需要遍历：

```
O(N)
```


例如：

```
Timer A
Timer B
Timer C
Timer D
```


寻找 Timer C：

```
A -> B -> C
```

需要线性扫描。


因此增加：

```cpp
unordered_map<uint64_t,size_t> m_timerIndex;
```


维护：

```
TimerId
    |
    v

heap 下标
```


例如：

```
m_heap


index       Timer
--------------------
0           id=5
1           id=3
2           id=8



m_timerIndex


id      index
------------
5        0
3        1
8        2

```


删除：

```cpp
cancel(id)
```


过程：

```
id
 |
 v
unordered_map

 O(1)

 |
 v

heap index

 |
 v

remove(index)

 O(logN)
```


最终：

```
O(logN)
```


# 四、最小堆实现


## 1. 数组模拟完全二叉树


堆使用 vector 保存：


```
index:

        0

     1     2

   3  4   5  6

```


对应关系：


父节点：

```cpp
(index - 1) / 2
```


左孩子：

```cpp
index * 2 + 1
```


右孩子：

```cpp
index * 2 + 2
```


# 五、插入 Timer


流程：


## 1. 插入数组尾部


例如：

```
      10
    /    \
  30      50

新增 5
```


先插入：

```
      10
    /    \
  30      50
 /
5
```


此时破坏堆性质。


## 2. 上滤 heapifyUp


比较：

```
5 < 30
```

交换：


```
      10
    /    \
   5      50
 /
30
```


继续比较：

```
5 < 10
```


最终：


```
       5
    /     \
  10       50
 /
30
```


恢复最小堆。


# 六、删除 Timer


删除任意节点时：

不能直接删除：

```
      10
    /    \
  20      30

删除20
```

否则破坏完全二叉树结构。


因此采用：

## 1. 尾节点覆盖


删除节点：

```
      10
    /    \
  20      30

```


交换最后节点：


```
      10
    /    \
  30
```


删除尾部。


## 2. 调整堆


替换后的节点可能：

- 比父节点小
- 比子节点大


因此需要判断：


如果：

```cpp
node < parent
```

执行：

```
heapifyUp()
```


如果：

```cpp
node > child
```

执行：

```
heapifyDown()
```


# 七、tick 执行流程


调用：

```cpp
TimerManager::tick()
```


流程：

```
获取当前时间

        |
        v

检查堆顶 Timer

        |
        |
        +---- 未到期
        |
        v

     已到期

        |
        v

删除堆顶

        |
        v

执行 callback

        |
        v

继续检查下一个 Timer

```


为什么只检查堆顶？


因为最小堆保证：


```
heap[0] <= 所有节点
```


如果：

```
heap[0] 未到期
```

那么：

```
其它 Timer 一定未到期
```


因此可以直接退出。


# 八、核心接口


## 添加 Timer


```cpp
TimerId add(
    uint64_t expire,
    Timer::Callback callback
);
```


示例：


```cpp
uint64_t now =
    std::chrono::duration_cast<
        std::chrono::milliseconds>
    (
        std::chrono::steady_clock::now()
        .time_since_epoch()
    )
    .count();


auto id = timer.add(
    now + 1000,
    []()
    {
        std::cout<<"timeout\n";
    });
```



## 取消 Timer


```cpp
void cancel(TimerId id);
```


示例：

```cpp
timer.cancel(id);
```


取消后：

- Timer 从堆中删除
- 不会执行 callback


## 执行到期任务


```cpp
timer.tick();
```


通常由事件循环周期调用：


```cpp
while(true)
{
    timer.tick();

    doOtherWork();
}
```


# 九、内部函数说明


## heapifyUp


作用：

向上恢复堆性质。


使用场景：

- 插入新 Timer


复杂度：

```
O(logN)
```



## heapifyDown


作用：

向下恢复堆性质。


使用场景：

- 删除 Timer
- 删除堆顶


复杂度：

```
O(logN)
```


## swapNode


普通 swap：


```cpp
std::swap()
```


但是 TimerManager 额外需要维护：

```
TimerId -> heap index
```


因此封装：

```cpp
swapNode()
```


保证：

```
堆数据
+
索引映射
```

始终一致。


# 十、特点


优点：

- 插入删除效率高
- 获取最近任务 O(1)
- 支持取消任意 Timer
- 实现简单，容易扩展


缺点：

- tick 精度依赖调用周期
- 大量 Timer 同时触发时 callback 会集中执行
- 不支持高精度纳秒级调度


# 十一、与时间轮比较


| |最小堆|时间轮|
|-|-|-|
|插入|O(logN)|O(1)|
|删除|O(logN)|O(1)|
|精度|高|较低|
|实现难度|简单|复杂|
|大量定时器|一般|优秀|


最小堆适合：

- 定时任务数量中等
- 需要较高精度


时间轮适合：

- 网络服务器
- 大量连接超时管理
- 百万级 Timer

