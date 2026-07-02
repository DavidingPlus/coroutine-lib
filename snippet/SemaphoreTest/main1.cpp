#include <iostream>
#include <vector>

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>


// int sem_init(sem_t *sem, int pshared, unsigned int value); // 创建信号量，pshared：0 用在线程，非 0 用在进程。
// int sem_wait(sem_t *sem);                                  // 对信号量加锁，调用一次，对信号量的值减 1，如果值为 0，就阻塞。
// int sem_post(sem_t *sem);                                  // 解锁一个信号量，调用一次，对信号量的值加 1。
// int sem_destroy(sem_t *sem);                               // 信号量销毁。

#define MAXNUM 2

// 最多允许两个下载任务同时运行。
sem_t semDownload;

// 下载任务。
struct Task
{
    int id;        // 任务类型。
    int sleepTime; // 模拟下载时间。
};

// 下载线程
void *download(void *arg)
{
    Task *task = static_cast<Task *>(arg);

    // 获取一个下载名额。
    sem_wait(&semDownload);

    printf("============== Downloading taskType %d ==============\n", task->id);

    sleep(task->sleepTime);

    printf("============== Finished taskType %d ==============\n", task->id);

    // 释放下载名额。
    sem_post(&semDownload);

    delete task;


    return nullptr;
}

int main()
{
    // 初始化信号量，最多允许 2 个线程同时下载。
    sem_init(&semDownload, 0, MAXNUM);

    std::vector<pthread_t> threads;

    int taskTypeId;

    while (true)
    {
        printf("\nInput task type (1~3, 0 to quit): ");

        if (1 != scanf("%d", &taskTypeId)) break;
        if (0 == taskTypeId) break;

        Task *task = new Task{.id = taskTypeId, .sleepTime = 0};

        switch (taskTypeId)
        {
            case 1:
                task->sleepTime = 5;
                break;

            case 2:
                task->sleepTime = 3;
                break;

            case 3:
                task->sleepTime = 1;
                break;

            default:
                printf("Invalid task type!\n");
                delete task;
                continue;
        }

        pthread_t tid;

        if (0 == pthread_create(&tid, nullptr, download, task))
        {
            threads.push_back(tid);
        }
        else
        {
            printf("Create thread failed!\n");
            delete task;
        }
    }

    printf("\nWaiting for all download tasks to finish...\n");

    // 等待所有线程结束。
    for (pthread_t tid : threads) pthread_join(tid, nullptr);

    sem_destroy(&semDownload);

    printf("All download tasks finished.\n");


    return 0;
}
