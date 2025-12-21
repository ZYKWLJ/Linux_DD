#include <stdio.h>
#include "b.h"
extern void b();
extern int c;
int main(void)
{
    printf("this is a test---%d\n", c);
    b();
    return 0; 
}