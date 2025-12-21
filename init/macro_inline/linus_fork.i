# 1 ".\\linus_fork.c"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 ".\\linus_fork.c"
int errno = 0;
# 15 ".\\linus_fork.c"
static inline int fork(void)
{
    long __res;
    __asm__ volatile("int $0x80" : "=a"(__res) : "0"(2));
    if (__res >= 0)
        return (int)__res;
    errno = -__res;
    return -1;
}
