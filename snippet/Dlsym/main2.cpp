#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>


int main()
{
    const char *path = getenv("HELLO_SO");
    printf("HELLO_SO: %s\n", path);

    // RTLD_LAZY 表示在对符号引用时才解析符号，但只对函数符号引用有效，而对于变量符号的引用总是在加载该动态库的时候立即绑定解析。
    void *handle = dlopen(path, RTLD_LAZY);
    if (!handle)
    {
        printf("dlopen failed: %s\n", dlerror());
        return -1;
    }

    // 获取函数地址。
    void (*hello)();

    hello = (void (*)())dlsym(handle, "hello");

    if (!hello)
    {
        printf("dlsym failed: %s\n", dlerror());
        return -1;
    }

    // 调用动态库函数。
    hello();

    // 获取变量地址
    int *value;

    value = (int *)dlsym(handle, "global");

    printf("global = %d\n", *value);

    dlclose(handle);
}
