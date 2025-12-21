# 一、作用：
这是linux0.11的主干，其他的任何子系统都是以这个为基础发展调用的。所以，本质上来说，我们讲Linux的执行过程，就是讲的这个main函数的执行过程。把它理清楚了，Linux0.11也就完全明白了！

# 二、按代码逐行解析

## (一)COW问题
### 1.COW
为了保证在内核态中， 执行fork时，会导致Copy On Write (COW，写时复制)的系统优化机制失效。这是因为早期 COW 只针对用户页表：Linux 0.11 的 COW 逻辑只在用户页表（user page table）中标记 “只读共享”，内核页表（kernel page table）没有这个逻辑 —— 因为内核内存被所有进程共享，设计上就没考虑对内核栈做 COW。

正因为内核栈没有 COW 保护，一旦**父 / 子进程对栈做操作**（比如函数调用的返回、栈指针修改），**就会互相覆盖栈数据，导致崩溃。** 所以注释才要求：

1. fork `必须内联`（消除`函数调用 / 返回的栈`操作）；
2. main () 在 `fork 后不能用栈`（避免任何栈修改）；
3. 直到子进程执行 `execve，切换到用户空间`，才会`脱离这个共享内核栈`的风险。因为此时，，切换到用户空间，子进程重新建立齐了自己的用户空间，此时 COW 的问题才会消失。

>内核栈是 **“每进程私有**但**内核态共享”** 的：每个进程有自己的内核栈，但 fork 在内核态执行时，**子进程还没完成初始化，会临时共享父进程的内核栈；**

进程私有但内核态共享，这就很无奈啊~~

总的来说，就是：

> 在内核空间执行 fork 时，由于写时复制机制失效，父 / 子进程会共享内核栈；为了避免栈操作导致的数据错乱，要求 main () 在 fork 后完全不使用栈 —— 因此必须把 fork（及相关函数）实现为内联，消除函数调用 / 返回带来的栈操作。

### 2.对应fork实现：
```c
static inline _syscall0(int,fork)
static inline _syscall0(int,pause)
static inline _syscall1(int,setup,void *,BIOS)
static inline _syscall0(int,sync)
```
这些函数都是宏实现的：这里 _syscall0举例子：
```c
#define _syscall0(type, name)                 \
    type name(void)                           \
    {                                         \
        long __res;                           \
        __asm__ volatile("int $0x80"          \
                         : "=a"(__res)        \
                         : "0"(__NR_##name)); \
        if (__res >= 0)                       \
            return (type)__res;               \
        errno = -__res;                       \
        return -1;                            \
    }
```

### 3.宏定义内联为啥能优化运行性能和避免使用栈？ 
那么为什么内联了就能优化运行性能和避免使用栈？ 
————答案是“内联(宏定义)直接是结果的复制粘贴，将运行时的性能损耗提前至了编译期，既然是直接得到的结果，那么也就不存在调用函数一说了。所以内联(宏定义)既能优化运行性能，又可以避免函数调用，也就避免了使用栈。”
a.c
```c
#include <stdio.h>

#define add1(a, b) ((a) + (b))

int main(void)
{
    printf("result=%d", add1(1, 4));
    return 0;
}
```
这是使用宏定义，内联函数的汇编

```s
	.file	"a.c"
	.text
	.def	__main;	.scl	2;	.type	32;	.endef
	.section .rdata,"dr"
.LC0:
	.ascii "result=%d\0"
	.text
	.globl	main
	.def	main;	.scl	2;	.type	32;	.endef
main:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	call	__main
	movl	$5, %edx    # 这就是直接在编译期就计算出来了1+4的值了！没有add函数的任何汇编，更别谈及栈的替换了
	leaq	.LC0(%rip), %rcx
	call	printf
	movl	$0, %eax
	leave
	ret
	.ident	"GCC: (x86_64-posix-sjlj-rev0, Built by MinGW-W64 project) 8.1.0"
	.def	printf;	.scl	2;	.type	32;	.endef
```

b.c
```c
#include <stdio.h>

#define add1(a, b) ((a) + (b))

int main(void)
{
    printf("result=%d", add1(1, 4));
    return 0;
}
```
这是使用函数调用生成的汇编 
```s
	.file	"b.c"
	.text
	.globl	add2
	.def	add2;	.scl	2;	.type	32;	.endef
add2:                       # 这是add2函数，有明确的函数调用栈帧和栈数据的变化！
	pushq	%rbp
	movq	%rsp, %rbp
	movl	%ecx, 16(%rbp)
	movl	%edx, 24(%rbp)
	movl	16(%rbp), %edx
	movl	24(%rbp), %eax
	addl	%edx, %eax
	popq	%rbp
	ret
	.def	__main;	.scl	2;	.type	32;	.endef
	.section .rdata,"dr"
.LC0:
	.ascii "result=%d\0"
	.text
	.globl	main
	.def	main;	.scl	2;	.type	32;	.endef
main:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	call	__main
	movl	$4, %edx
	movl	$1, %ecx
	call	add2        # 这里就是会调用add2函数，栈会有变化。不想上面一样，直接宏定义替换。
	movl	%eax, %edx
	leaq	.LC0(%rip), %rcx
	call	printf
	movl	$0, %eax
	leave
	ret
	.ident	"GCC: (x86_64-posix-sjlj-rev0, Built by MinGW-W64 project) 8.1.0"
	.def	printf;	.scl	2;	.type	32;	.endef
```

显然，使用宏定义编译出来的汇编代码很简洁，并且关于add宏定义，直接替换成了结果`5`,但是传统的函数，就需要在运行期进行出栈入栈等等操作计算。
### 4.宏定义的优化本质
> 宏定义，本质上是将整个程序的性能提升过程前移，将运算推前到编译期优化，从而保全了运行期的极致性能。

## (二)自己模仿linus实现的用户层宏定义加法函数
```c
#include <stdio.h>
#include <errno.h>  // 补充errno头文件

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
static inline _syscall2(int, add, int, a, int, b)

// 4. 主函数：调用生成的add函数，而非直接调用宏
int main(void) {
    int result = add(1, 5);  // 调用宏生成的add函数
    printf("result=%d\n", result);    
    return 0;
}
```

预处理后，完成宏替换，如下：
```c
static inline int add(int a, int b) { long __res; __res = (long)(a + b); if (__res >= 0) return (int)__res; //得到了add函数了吧！
# 20 ".\\linus_define.c" 3
             (*_errno()) 
# 20 ".\\linus_define.c"
             = -__res; return -1; }


int main(void) {
    int result = add(1, 5);
    printf("result=%d\n", result);
    return 0;
}
```



## (三)系统调用精讲
### 1.fork调用详细转化
详细讲解这句话：
```c
static inline _syscall0(int,fork)
```
===
linus对于系统调用的实现如下：
宏定义：
```c
#define __NR_fork 2
```
```c
#define _syscall0(type, name)                 \
    type name(void)                           \
    {                                         \
        long __res;                           \
        __asm__ volatile("int $0x80"          \
                         : "=a"(__res)        \
                         : "0"(__NR_##name)); \
        if (__res >= 0)                       \
            return (type)__res;               \
        errno = -__res;                       \
        return -1;                            \
    }
```

所以调用了这个`_syscall0(int,fork)`函数，会生成：如下.i文件：
```c
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
```
由此可以看到，这里的`系统调用`马上就`变出来了！`很牛逼！
### 2.fork转化后源码详细讲解

详解如下：
- ✅️本质是通过 `int $0x80` 触发`内核中断`，调用`内核编号为 2` 的 `fork 系统调用`；
- ✅️接收`内核返回的结果`，处理错误（`设置 errno`）后返回`子进程 PID（成功）`或 `-1（失败）`；
- ✅️static inline 修饰符则是为了`消除函数调用的栈操作`（呼应我们之前聊的内核态 fork 栈安全问题）;

```c
__asm__ volatile("int $0x80" : "=a"(__res) : "0"(2));
```

|部分|含义|
|-|-|
|__asm__|       `GCC 内嵌汇编`的`关键字`，告诉编译器这里是`汇编代码（等价于 asm）`；|
|volatile|      `禁止编译器优化`这段汇编代码（必须执行，不能被省略 / 重排），`保证系统调用的原子性`；|
|"int $0x80"|   x86 架构的中断指令：触发 0x80 号软中断，跳转到内核的系统调用入口；|
|: "=a"(__res)| 输出约束：`"=a" 表示将 %eax 寄存器的值写入变量 __res`（`a 对应 %eax，= 表示只写`）；|
|: "0"(2)|      输入约束："0" 表示`复用第一个输出约束的寄存器`（即 `%eax`），将数值 2 `写入 %eax`；|

```c
if (__res >= 0) return (int)__res;
```
- 内核返回的 `__res >= 0` 表示`系统调用成功`：
- 父进程中，__res 是`子进程的 PID（正数）`；
- 子进程中，__res 是 0（这是 fork 的特性：`一次调用，两次返回`；
- 将 long 型的 __res 强转为 int 并返回（`PID 是整型`）。

//保持疑问？父子进程是怎么体现两次返回的？

====
```c
errno = -__res;
```
- 若 __res < 0，表示`系统调用失败`：内核返回的是`负数错误码`（比如 `-EAGAIN` 表示资源不足）；
- 将 __res 取反后赋值给 `errno`（errno 是`全局变量`，存储`用户态可识别的错误码`，比如 -(-EAGAIN) = EAGAIN）。

===
```c
return -1;
```
**系统调用失败时，返回 -1**（这是 fork 函数的约定：**失败返回 -1，成功返回子进程 PID/0**）。

### 3.为何说一次调用，父子进程两次返回？
https://www.doubao.com/thread/wef29de2fdea1b4a3
![3.为何说一次调用，父子进程两次返回？](3.为何说一次调用，父子进程两次返回？)


