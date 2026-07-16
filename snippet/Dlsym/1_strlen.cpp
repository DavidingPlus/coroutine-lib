#include <stdio.h>


#ifndef __USE_GNU
#define __USE_GNU
#endif


#include <dlfcn.h>


// strlen 函数类型。
using strlenFunc = size_t (*)(const char *);

// 保存 libc 中真正的 strlen。
static strlenFunc realStrlen = nullptr;


// 我们自己的 strlen。
extern "C"
{
    /*
     * hook 函数内部不要随意调用复杂的 C++ 库函数。
     *
     * 例如：
     *
     *     std::cout << "hook strlen";
     *
     * std::cout 属于 C++ iostream 库，内部依赖：
     *
     *     - C++ runtime 初始化
     *     - locale
     *     - 缓冲区管理
     *     - 动态内存分配
     *
     * 而这些操作在动态链接初始化阶段可能再次调用 strlen，
     * 导致：
     *
     *     strlen()
     *        |
     *        ↓
     *     std::cout
     *        |
     *        ↓
     *     strlen()
     *        |
     *        ↓
     *     无限递归
     *
     * 最终导致栈溢出 segmentation fault。
     *
     * printf 相对简单，通常不会触发这种递归，因此在这个例子中可以正常工作。但严格来说，hook 函数中仍应尽量避免依赖复杂 libc/C++ 功能。
     */
    size_t strlen(const char *str)
    {
        printf("%s hook strlen\n", __FILE__);

        /*
         * 第一次调用时寻找真正的 strlen。
         *
         * RTLD_DEFAULT 表示按默认的顺序搜索共享库中符号 symbol 第一次出现的地址。
         * RTLD_NEXT 表示在当前库以后按默认的顺序搜索共享库中符号 symbol 第一次出现的地址。
         *
         * 当前：libhook.so
         * 后面：libc.so
         */
        if (!realStrlen)
        {
            // dlsym函数的功能就是可以从共享库（动态库）中获取符号（全局变量与函数符号）地址，通常用于获取函数符号地址，这样可用于对共享库中函数的包装。
            realStrlen = reinterpret_cast<strlenFunc>(dlsym(RTLD_NEXT, "strlen"));

            if (!realStrlen)
            {
                printf("dlsym strlen failed: %s\n", dlerror());
                return 0;
            }

            printf("real strlen address = %p", reinterpret_cast<void *>(realStrlen));
        }


        // 调用 libc 的 strlen。
        return realStrlen(str);
    }
}
