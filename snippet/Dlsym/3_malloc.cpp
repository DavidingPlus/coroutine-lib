#include <stdio.h>
#include <stdlib.h>

#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <dlfcn.h>


// 保存真实 malloc 函数类型。对应 libc 中的：void *malloc(size_t size)。
typedef void *(*malloc_t)(size_t size);

// 保存真实 free 函数类型。对应 libc 中的：void free(void *ptr)。
typedef void (*free_t)(void *ptr);


// 指向 libc 中真正的 malloc/free。
static malloc_t realMalloc = nullptr;
static free_t realFree = nullptr;


// 防止递归调用。
// 例如:
// hook malloc()
//       |
//       +-- printf()
//              |
//              +-- malloc()
//
// 如果不处理，会无限进入自己的 malloc()。使用 thread_local 保证每个线程独立。
static thread_local bool mallocHooking = false;
static thread_local bool freeHooking = false;


// 动态库加载时自动执行。比 main() 更早执行。这样可以保证程序真正调用 malloc 前，已经获取 libc 中 malloc 的地址。
__attribute__((constructor)) static void initHook()
{
    realMalloc = (malloc_t)dlsym(RTLD_NEXT, "malloc");

    if (!realMalloc)
    {
        printf("load malloc failed: %s\n", dlerror());
        exit(1);
    }

    realFree = (free_t)dlsym(RTLD_NEXT, "free");

    if (!realFree)
    {
        printf("load free failed: %s\n", dlerror());
        exit(1);
    }
}


// hook malloc。程序调用 malloc() 时，实际上先进入这里。
extern "C" void *malloc(size_t size)
{
    // 防止递归进入 hook。
    if (mallocHooking) return realMalloc(size);

    mallocHooking = true;

    printf("malloc size=%zu\n", size);

    // 调用真正的 libc malloc。
    void *ptr = realMalloc(size);

    mallocHooking = false;


    return ptr;
}

// hook free。程序调用 free() 时，先进入这里。
extern "C" void free(void *ptr)
{
    if (freeHooking)
    {
        realFree(ptr);
        return;
    }

    freeHooking = true;

    printf("free ptr=%p\n", ptr);

    // 调用真正的 libc free。
    realFree(ptr);

    freeHooking = false;
}
