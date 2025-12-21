#include <stdio.h>
#include <errno.h>  // 补充errno头文件

// 1. 定义自定义加法系统调用号（模拟内核unistd.h）
#define __NR_add  123  // 选一个未被内核占用的编号

// 2. 保留你提供的_syscall2宏（仅补充注释）
#define _syscall2(type, name, type1, arg1, type2, arg2) \
type name(type1 arg1, type2 arg2) { \
    long __res; \
    /* 模拟系统调用：替换int $0x80，避免调用不存在的内核系统调用 */ \
    /* 真实场景中此处是int $0x80，这里直接计算加法（仅演示） */ \
    __res = (long)(arg1 + arg2); \
    /* 保留原宏的返回值逻辑 */ \
    if (__res >= 0) return (type)__res; \
    errno = -__res; return -1; \
}

// 3. 用_syscall2宏生成add系统调用函数（核心：先定义，再调用）
static inline _syscall2(int, add, int, a, int, b)//这里会直接被替换成add 函数！

// 4. 主函数：调用生成的add函数，而非直接调用宏
int main(void) {
    int result = add(1, 5);  // 调用宏生成的add函数，看似没有add函数，其实是有的，宏生成的预处理才能看到。
    printf("result=%d\n", result);    
    return 0;
}

//这就是预处理的结果，可以看见add函数了吧！
// gcc .\linus_define.c -E -o .\linus_define.i
/*
static inline int add(int a, int b) { long __res; __res = (long)(a + b); if (__res >= 0) return (int)__res; 
# 20 ".\\linus_define.c" 3
             (*_errno()) 
# 20 ".\\linus_define.c"
             = -__res; return -1; }


int main(void) {
    int result = add(1, 5);
    printf("result=%d\n", result);
    return 0;
}
*/