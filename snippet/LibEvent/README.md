# libevent 学习笔记

参考链接：[https://zhuanlan.zhihu.com/p/709822756](https://zhuanlan.zhihu.com/p/709822756)。

结合代码：

- `main1.cpp`
- `main2.cpp`
- `main3_server.cpp`
- `main3_client.cpp`

这组代码基本覆盖了 `libevent` 入门时最重要的几个问题：

- 定时器怎么注册
- 一个事件循环怎么同时处理多种事件
- socket 为什么要配合非阻塞使用
- 服务端为什么通常只长期监听读事件，不长期监听写事件
- 应用层为什么要自己维护发送缓冲区

# 一、libevent 是干什么的

`libevent` 不是网络协议库，它不替你完成 `socket`、`bind`、`listen`、`accept`、`read`、`write` 这些系统调用。

它主要解决的是另一件事：

```text
哪个 fd / 定时器 / 信号 现在就绪了？
就绪之后，应该回调哪个函数？
```

可以把它理解成一个事件调度框架。

核心流程：

```text
创建 event_base
        |
注册 event
        |
event_add()
        |
event_base_dispatch()
        |
等待事件发生
        |
调用对应回调函数
```

# 二、核心对象

## 1. event_base

`event_base` 是整个事件循环的管理对象。

代码里最常见的写法：

```cpp
event_base *base = event_base_new();
```

它可以理解为：

- 事件循环上下文
- 所有事件的挂载点
- 底层复用机制的统一入口

在 Linux 下，底层通常会落到 `epoll`。

## 2. event

`event` 表示“对某个事件源的监听描述”。

典型创建方式：

```cpp
event *ev = event_new(base, fd, events, cb, arg);
```

参数含义：

| 参数     | 含义                              |
| -------- | --------------------------------- |
| `base`   | 事件所属的 `event_base`           |
| `fd`     | 监听的文件描述符；定时器可传 `-1` |
| `events` | 监听的事件类型                    |
| `cb`     | 回调函数                          |
| `arg`    | 传给回调函数的自定义参数          |

回调原型：

```cpp
void callback(evutil_socket_t fd, short events, void *arg);
```

## 3. 常见事件类型

你这份代码里已经用到了最常见的几个：

| 事件         | 含义                       |
| ------------ | -------------------------- |
| `EV_TIMEOUT` | 超时事件                   |
| `EV_READ`    | 可读事件                   |
| `EV_WRITE`   | 可写事件                   |
| `EV_PERSIST` | 持久监听，触发后不自动移除 |

最常见的组合：

```cpp
EV_READ | EV_PERSIST
```

表示：

```text
持续监听某个 fd 的可读状态
```

# 三、`main1.cpp`：最小定时器示例

这个文件只做了一件事：

```text
1 秒后执行一次 timerCb
```

核心代码：

```cpp
event_base *base = event_base_new();

event *ev = event_new(
    base,
    -1,
    EV_TIMEOUT,
    timerCb,
    nullptr);

timeval tv = {
    .tv_sec = 1,
    .tv_usec = 0,
};

event_add(ev, &tv);
event_base_dispatch(base);
```

## 1. 为什么定时器的 fd 是 `-1`

因为定时器不是基于真实文件描述符触发的。

所以这里：

```cpp
event_new(base, -1, EV_TIMEOUT, timerCb, nullptr);
```

表示注册一个超时事件，而不是监听某个 socket。

---

## 2. `event_add(ev, &tv)` 的作用

这里不只是“把事件加进去”，还同时指定了超时时间：

```cpp
1 秒后触发
```

流程可以画成：

```text
event_new()
    |
创建一个 timeout event
    |
event_add(ev, &tv)
    |
把 1 秒超时信息注册进 event loop
    |
event_base_dispatch()
    |
1 秒后执行 timerCb
```

## 3. 这个例子说明了什么

`libevent` 不只处理网络 IO，也能管理定时器。

这是后面理解网络服务器的基础，因为网络程序里经常同时需要：

- IO 事件
- 超时事件
- 信号事件

# 四、`main2.cpp`：一个事件循环同时管理 stdin 和定时器

这个例子把两类事件放进了同一个 `event_base`：

- 标准输入可读事件
- 周期定时器事件

核心代码：

```cpp
event *stdinEv = event_new(
    base,
    STDIN_FILENO,
    EV_READ | EV_PERSIST,
    readCb,
    nullptr);

event *timerEv = event_new(
    base,
    -1,
    EV_TIMEOUT | EV_PERSIST,
    timerCb,
    nullptr);
```

## 1. `stdinEv` 在做什么

```cpp
EV_READ | EV_PERSIST
```

表示持续监听标准输入是否可读。

一旦终端输入内容，事件循环会调用：

```cpp
readCb()
```

然后在回调里执行：

```cpp
read(fd, buffer, sizeof(buffer));
```

## 2. `timerEv` 在做什么

```cpp
EV_TIMEOUT | EV_PERSIST
```

这里和 `main1.cpp` 的差别是多了 `EV_PERSIST`。

这意味着：

```text
触发一次后不删除
继续保留在事件循环里
下一个周期继续触发
```

所以它是一个周期性定时器。

## 3. 这个例子最关键的意义

它说明：

```text
一个 event_base 可以统一管理多种事件源
```

也就是说，事件循环不是“网络服务器专用技巧”，而是一种统一调度异步事件的方法。

流程如下：

```text
event_base
   |
   +---- stdin 可读事件
   |
   +---- 1 秒周期定时器
   |
event_base_dispatch()
   |
谁先就绪就先处理谁
```

# 五、`main3_server.cpp`：非阻塞 Echo 服务器

这个文件是整组代码里最核心的部分。

它做的是一个最基本的 Echo 服务器：

```text
客户端发什么
服务端原样回什么
```

但真正有价值的不是 Echo 本身，而是它把以下问题都串起来了：

- 监听 socket 的注册
- 新连接的 accept
- 每个连接单独维护上下文
- 非阻塞 read / write
- 写缓冲区管理
- 写事件按需开启

## 1. 服务端整体流程

主流程很标准：

```text
socket()
   |
bind()
   |
listen()
   |
setNonblock(listenFd)
   |
注册 listenFd 的读事件
   |
event loop
   |
accept 新连接
   |
给每个 clientFd 注册读事件和写事件
```

对应代码：

```cpp
int listenFd = socket(AF_INET, SOCK_STREAM, 0);
bind(listenFd, ...);
listen(listenFd, SOMAXCONN);
setNonblock(listenFd);
```

然后把监听 socket 挂进 `libevent`：

```cpp
event *listenEv = event_new(
    base,
    listenFd,
    EV_READ | EV_PERSIST,
    acceptCb,
    base);
```

## 2. 为什么监听 socket 监听的是 `EV_READ`

因为对监听 socket 来说：

```text
“可读” 不表示有数据可读
而表示 accept 队列里有新连接可取
```

所以：

```cpp
EV_READ | EV_PERSIST
```

就是长期监听新连接到来。

## 3. `acceptCb` 为什么要 `while(true)`

代码里不是只 `accept` 一次，而是一直取到 `EAGAIN` 为止：

```cpp
while (true)
{
    int clientFd = accept(fd, ...);
    if (clientFd < 0)
    {
        if (EAGAIN == errno || EWOULDBLOCK == errno) break;
        ...
    }
    ...
}
```

这是非阻塞服务端里的典型写法。

原因：

```text
一次监听 fd 可读
可能对应多个连接已经排队到 accept 队列
```

如果只取一个连接就返回，其余连接还要等下一次事件循环。

所以正确流程是：

```text
listenFd 可读
    |
进入 acceptCb
    |
循环 accept
    |
直到返回 EAGAIN
    |
说明当前没有更多连接
```

## 4. 为什么要给每个客户端维护 `ClientContext`

代码里定义：

```cpp
struct ClientContext
{
    event *readEv = nullptr;
    event *writeEv = nullptr;
    std::string outputBuffer;
};
```

这个结构非常重要。

事件驱动程序不是按“调用栈上下文”工作的，而是按“状态对象”工作的。一个连接后面会经历很多次事件触发，所以必须把状态挂在连接对象上。

这里保存了三类信息：

| 字段           | 作用                   |
| -------------- | ---------------------- |
| `readEv`       | 该连接的读事件         |
| `writeEv`      | 该连接的写事件         |
| `outputBuffer` | 待发送但还没发完的数据 |

## 5. 为什么客户端 fd 要设为非阻塞

在 `acceptCb` 里：

```cpp
setNonblock(clientFd);
```

原因很直接：

如果客户端 socket 是阻塞的，那么某次 `read()` 或 `write()` 卡住以后，整个事件循环就会被拖死。

而事件驱动模型的前提就是：

```text
所有 IO 调用都要尽量“立即返回”
做不了就返回 EAGAIN
下次事件来了再继续
```

# 六、读事件为什么适合长期监听

客户端读事件的创建方式：

```cpp
ctx->readEv = event_new(
    base,
    clientFd,
    EV_READ | EV_PERSIST,
    readCb,
    ctx);
```

含义：

```text
长期监听这个连接什么时候有数据到达
```

这很合理，因为：

- 客户端什么时候发数据是不确定的
- 只要连接还活着，后面就可能继续发
- `EV_PERSIST` 可以保证触发一次后继续保留监听

所以读事件通常适合常驻。

# 七、写事件为什么不能常驻监听

这份代码最值得记住的点就在这里。

写事件的创建方式是：

```cpp
ctx->writeEv = event_new(
    base,
    clientFd,
    EV_WRITE,
    writeCb,
    ctx);
```

注意两点：

- 没有 `EV_PERSIST`
- 创建后没有立刻 `event_add`

这不是随便写的，而是有明确原因。

## 1. socket 大多数时候都是可写的

对 TCP 已连接 socket 来说，只要内核发送缓冲区还有空间，它通常就是“可写”的。

所以如果长期监听：

```cpp
EV_WRITE | EV_PERSIST
```

会出现这种情况：

```text
应用层其实没有数据要发
但 socket 长期处于可写状态
event loop 就会反复触发 writeCb
```

结果：

- 回调空转
- CPU 白白消耗

## 2. 正确做法：按需开启写事件

这份代码的策略是：

```text
outputBuffer 为空
    -> 不监听 EV_WRITE

outputBuffer 从空变成非空
    -> 开始监听 EV_WRITE

数据全部发完
    -> 立即取消 EV_WRITE
```

对应 `readCb` 里的逻辑：

```cpp
bool needWrite = ctx->outputBuffer.empty();
ctx->outputBuffer.append(buffer, n);

if (needWrite) event_add(ctx->writeEv, nullptr);
```

也就是说，只有从“无待发送数据”变成“有待发送数据”的那个时刻，才把写事件挂进去。

## 3. `writeCb` 在做什么

核心逻辑：

```cpp
int n = write(fd, ctx->outputBuffer.data(), ctx->outputBuffer.size());
```

分几种情况：

### 情况一：成功发送了一部分或全部

```cpp
ctx->outputBuffer.erase(0, n);
```

如果发完了：

```cpp
event_del(ctx->writeEv);
```

如果还没发完：

```cpp
event_add(ctx->writeEv, nullptr);
```

### 情况二：返回 `EAGAIN`

表示当前内核发送缓冲区满了，暂时发不出去。

这不是严重错误，而是非阻塞 IO 的正常状态。

此时：

```text
保留 outputBuffer
等待下次 EV_WRITE 再继续发
```

### 情况三：真正写失败

这时就需要：

- 删除事件
- 释放事件
- 关闭 fd
- 删除上下文

# 八、为什么一定要有 `outputBuffer`

这是这份 Echo 代码真正进入“工程思维”的地方。

很多入门代码会写成：

```cpp
read(...)
write(...)
```

看起来很简单，但在非阻塞 socket 里这并不安全。

原因：

```text
一次 write 不保证把全部数据发送完
```

可能出现：

- 只发了一部分
- 一字节都没发出去，直接返回 `EAGAIN`

如果没有应用层发送缓冲区，那么剩余数据就丢了。

所以这里的：

```cpp
std::string outputBuffer;
```

本质上是在做：

```text
应用层待发送队列
```

流程如下：

```text
readCb 收到客户端数据
        |
追加到 outputBuffer
        |
开启写事件
        |
writeCb 尝试发送
        |
已发送部分从 outputBuffer 删除
        |
没发完则继续等下次可写
```

# 九、`readCb` 的三种返回情况

代码里对 `read()` 的返回值区分得很清楚：

## 1. `n > 0`

表示收到数据。

然后：

- 打印收到的内容
- 追加到 `outputBuffer`
- 必要时开启写事件

## 2. `n == 0`

表示：

```text
客户端主动关闭连接
```

这时候要做完整清理：

- `event_del(readEv)`
- `event_del(writeEv)`
- `close(fd)`
- `event_free(readEv)`
- `event_free(writeEv)`
- `delete ctx`

## 3. `n < 0`

如果是：

```cpp
EAGAIN == errno || EWOULDBLOCK == errno
```

表示当前没有数据可读，直接返回即可。

否则就是真正出错，需要回收资源。

# 十、`main3_client.cpp`：它本质上是一组测试

这个文件名叫 `client`，但内容其实更接近测试代码。

它定义了一个 `EchoClient`，然后通过 `gtest` 写了多组用例，分别验证服务端在不同场景下是否正常。

## 1. `BasicEcho`

验证最基础的回显功能：

```text
发 hello libevent
收 hello libevent
```

## 2. `LargeMessage`

验证较大数据回显是否完整：

```text
64 KB
```

这个测试的意义是检查：

- 服务端是否正确处理部分写
- `outputBuffer` 逻辑是否生效

## 3. `MultipleClient`

顺序创建多个客户端，检查服务端能否反复处理不同连接。

## 4. `ConcurrentClients`

50 个线程并发连接。

这主要在验证：

- 事件循环能否处理并发连接
- `acceptCb` 和每连接上下文是否正常工作

## 5. `ManySmallMessages`

同一个连接连续发送很多小消息。

这类测试适合观察：

- 频繁读写时是否稳定
- Echo 是否会错乱

## 6. `HugeMessage`

100 MB 消息。

这个测试很重，但非常能检验：

- 大量数据下发送缓冲区是否正确
- 服务端是否存在逻辑性丢数据

# 十一、这组代码里最值得记住的结论

## 1. libevent 管的是“事件分发”，不是业务协议

它负责告诉你：

```text
哪个事件到了
```

但不会替你处理：

- 业务消息边界
- 应用层发送队列
- 连接状态管理

这些都要自己做。

## 2. 事件驱动通常必须配合非阻塞 IO

否则某个 `read/write/accept` 阻塞住，整个事件循环就失效了。

## 3. 读事件和写事件的处理方式不一样

| 事件       | 常见策略           |
| ---------- | ------------------ |
| `EV_READ`  | 常驻监听           |
| `EV_WRITE` | 按需开启，发完关闭 |

这是写事件处理中最关键的经验。

## 4. 非阻塞 IO 里，`EAGAIN` 是正常状态

它不表示程序坏了，只表示：

```text
现在做不了
等下次事件再继续
```

## 5. 一个连接就是一个状态机

连接建立后，不是一次函数调用就结束，而是在很多轮事件中不断推进。

所以需要：

- `ClientContext`
- `readEv`
- `writeEv`
- `outputBuffer`

来保存连接状态。

