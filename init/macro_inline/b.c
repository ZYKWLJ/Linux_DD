#include <stdio.h>
int add2(int a, int b)
{
    return a + b;
}
int main(void)
{
    printf("result=%d", add2(1, 4));
    return 0;
}