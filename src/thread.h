#ifndef _COROUTINE_THREAD_H_
#define _COROUTINE_THREAD_H_

#include <iostream>
#include <thread>
#include <functional>
#include <string>

#include "semaphore.h"


class Thread
{

public:

    Thread(std::function<void()> cb, const std::string &name);

    ~Thread();

    pid_t getId() const { return m_id; }

    const std::string &getName() const { return m_name; }

    void join();


    static pid_t GetThreadId(); // 获取系统分配的线程 id。

    static Thread *GetThis(); // 获取当前所在线程。

    static const std::string &GetName(); // 获取当前线程的名字。

    static void SetName(const std::string &name); // 设置当前线程的名字。


private:

    static void *run(void *arg); // 线程函数。


    pid_t m_id = -1; // 进程的 id。

    pthread_t m_thread = 0; // 线程。

    std::function<void()> m_cb; // 线程需要运行的函数。

    std::string m_name; // 线程的 name。

    Semaphore m_semaphore; // 引入信号量的类来完成线程的同步创建。
};


#endif
