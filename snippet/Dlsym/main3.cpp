#include <stdio.h>
#include <stdlib.h>


int main()
{
    // 故意不释放 p2，用于模拟内存泄漏。
    // malloc 2 次，free 1 次。可以通过日志发现泄漏。
    void *p1 = malloc(10);
    void *p2 = malloc(20);

    free(p1);
}
