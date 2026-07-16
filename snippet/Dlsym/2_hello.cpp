#include <stdio.h>


extern "C"
{
    int global = 666;


    void hello()
    {
        printf("hello xuedaon\n");
    }
}
