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

> 即，为什么一次 fork() 调用会有两次返回，而且`父进程返回子进程 PID`、`子进程返回 0`？

#### 1.核心结论
fork() 的 “两次返回”**不是函数本身返回了两次**，而是：**内核在创建子进程时**，会让**父进程和子进程共享同一份用户态代码执行流**，但**通过修改寄存器（%eax）的值**，让父子进程在 __asm__ 执行完后，走到**同一个 if 判断时**，得到不同的结果，最终表现为 **“一次调用，两次返回”**。

#### 2.fork的实现详细讲述
代码如下：fork实现：
```c
static inline _syscall0(int,fork)
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
从这里可以看到，我们fork函数，通过`int $0x80`触发`内核中断`，调用`内核编号为 2` 的 `fork 系统调用`；
那么`int $0x80`中断的具体处理函数是什么呢？——在![kernel/sched.c](../kernel/sched.c)里面有定义：

#### 3.int $0x80触发，会发生什么？
##### 1.设置0x80的中断函数
我们在进程初始化sched_init(void)函数中，设置了系统调用0x80的中断处理函数，具体如下：
```c
void sched_init(void)
{
    int i;
    struct desc_struct *p;

    if (sizeof(struct sigaction) != 16)
        panic("Struct sigaction MUST be 16 bytes");
    set_tss_desc(gdt + FIRST_TSS_ENTRY, &(init_task.task.tss));
    set_ldt_desc(gdt + FIRST_LDT_ENTRY, &(init_task.task.ldt));
    p = gdt + 2 + FIRST_TSS_ENTRY;
    for (i = 1; i < NR_TASKS; i++)
    {
        task[i] = NULL;
        p->a = p->b = 0;
        p++;
        p->a = p->b = 0;
        p++;
    }
    /* Clear NT, so that we won't have troubles with that later on */
    __asm__("pushfl ; andl $0xffffbfff,(%esp) ; popfl");
    ltr(0);
    lldt(0);
    outb_p(0x36, 0x43);         /* binary, mode 3, LSB/MSB, ch 0 */
    outb_p(LATCH & 0xff, 0x40); /* LSB */
    outb(LATCH >> 8, 0x40);     /* MSB */
    set_intr_gate(0x20, &timer_interrupt);
    outb(inb_p(0x21) & ~0x01, 0x21);
    set_system_gate(0x80, &system_call); //主要看这里！定义了系统调用的中断处理！
}
```
我们看到最后一行：
```c
set_system_gate(0x80, &system_call); 
```
显然，含义就是设置了中断 0x80 的处理函数为 system_call，顾名思义为：系统调用！。
那么我们的`set_system_gate`函数的具体实现是什么呢？

##### 2.set_system_gate的实现
在我们的![include/asm/system.h](../include/asm/system.h)文件中，有定义`set_system_gate(n,addr)`函数：
```c
#define set_system_gate(n,addr) \
	_set_gate(&idt[n],15,3,addr) //描述的是，设置中断门n，类型为32位陷阱门，特权级为3，处理函数为addr
```
这里直接说_set_gate的作用，他是一个封装好的宏，用于设置中断门。参数分别表示：
`_set_gate(gate_addr,type,dpl,addr) `

- `gate_addr`：中断门的地址，表示写入**哪个中断描述符表（IDT）中的条目**。
- `type`：中断门的类型：门描述符的类型标识（`这是在x86 架构硬件层定义的，我们只能遵守`）：
- 15 = 0b1111 → 32 位`陷阱门`（Trap Gate）
- 其他常见值：14=32 位`中断门`（Interrupt Gate）
- `dpl`：中断门的特权级，决定了**哪些特权级**的代码可以触发该中断。
- `addr`：**中断处理函数的地址**，即当中断发生时，**CPU 会跳转到这个地址执行中断处理代码。**

##### 3._set_gate的源码

有了函数讲解，再来讲解具体实现，`_set_gate`函数的实现如下：
```c
#define _set_gate(gate_addr,type,dpl,addr) \
__asm__ ("movw %%dx,%%ax\n\t" \
	"movw %0,%%dx\n\t" \
	"movl %%eax,%1\n\t" \
	"movl %%edx,%2" \
	: \
	: "i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
	"o" (*((char *) (gate_addr))), \
	"o" (*(4+(char *) (gate_addr))), \
	"d" ((char *) (addr)),"a" (0x00080000))
```
逐行解释：

这里就是直接向硬件编程了，需要结合`IDT`和`IDT表项`结构进行讲解，所以我们补充一下IDT和IDT表项的源码定义：

###### 1.中断描述符表

###### 1.1 idt_descr定义
idtr描述符寄存器定义：
在![head.s](../boot_linux版/head.s)文件中，有定义`idt_descr`和`idt`的汇编层面定义：
```s
idt_descr:
	.word 256*8-1		# idt contains 256 entries
	.long idt
```
显然，这里给出了`idt`的汇编层面定义，首先指出，大小一共是256*8-1，因为一个`IDT表项`就是8字节。
其次，这里的idt表示中断描述符表的地址

###### 1.2 idt定义
以下代码，在内存中预留出一块`对齐到 8 字节边界`、包含 `256 个 8 字节表项`的`连续内存区域`，作为中断描述符表（IDT）的存储空间，并将所有表项`初始化为 0`。
```s
	.align 8
idt:	
    .fill 256,8,0		# idt is uninitialized
```
显然，这就是为其IDT和条目保存内存位置。


###### 1.3 C语言层面对idt、gdt的引用与细致解释
同时，在我们的![include/linux/head.h](../include/linux/head.h)文件中，有引用了`idt`的结构体：
> 注意，这里的引用是完完全全来自上面的![boot/head.s](../boot_linux版/head.s)里面定义的,因为汇编层面的东西不好操作，所以需要在C层面进行定义。其次，汇编层面仅仅定义了中断描述符的大小，没有定义细节，具体比较粗，而C语言层面定义了具体的中断描述符结构体，是细化的解释与实现。

```c
typedef struct desc_struct {
	unsigned long a,b; // 中断描述符，4+4=8字节。
} desc_table[256];

extern desc_table idt,gdt;
```
我们也发现，C语言中的解释，idt和gdt的表项 unsigned long 类型固定占 4 字节(Linux 0.11 对应的编译环境（gcc + x86 32 位）下)，因此 a 和 b 各占 4 字节，整个结构体总大小刚好 8 字节，完美匹配 x86 门描述符的长度。
由此，**也和汇编层面定义的结构一致。**

> 对比上面head.s的`.fill 256,8,0`,只定义了大小；而这里的结构体定义`unsigned long a,b;`，则是细化的解释与实现。

> 这里，如果你使用vscode或者其他编辑器，找不到idt的定义的，因为点击+跳转只在.c文件中进行，而idt的定义在.s文件中。
这也说明了，idt的定义是在汇编层面进行的，而C语言层面只是进行了引用。

###### 2.中断描述符
C语言定义：
```c
typedef struct desc_struct {
	unsigned long a,b; // 中断描述符，4+4=8字节。
} desc_table[256];
```
中断描述符图像：

![中断描述符图像](image.png)

可以看到，我们的中断描述符里面，主要是要找到段选择子和偏移地址。
那么这里的段选择子还会走我们的gdt过程，找到对应的段描述符，在找到对应的段基址。在于偏移地址相加，得到我们的中断处理函数地址！
所以，可以说，中断处理函数，只不过是在代码段寻址的基础上封装了一层中断描述符罢了。 

> 故，**中断只是触发了一次 “特殊的代码段寻址”，底层还是段式内存管理的核心规则。**

##### 4.结合idt结构，剖析_set_gate的源码
好了，终于把底层硬件知识都梳理完了，我们接下来再来看`_set_gate的源码`就简单多了！

```c
#define _set_gate(gate_addr,type,dpl,addr) \
__asm__ ("movw %%dx,%%ax\n\t" \
	"movw %0,%%dx\n\t" \
	"movl %%eax,%1\n\t" \
	"movl %%edx,%2" \
	: \
	: "i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
	"o" (*((char *) (gate_addr))), \
	"o" (*(4+(char *) (gate_addr))), \
	"d" ((char *) (addr)),
    "a" (0x00080000))
```
##### 4.0\n\t的含义
这里的`\n\t`定义如下：
- \n（换行符）：

- 告诉汇编器 “上一条指令结束，下一条是新指令”；

- \t（制表符）：

- 纯格式美化，让汇编代码缩进对齐，提升可读性（非必需，但内核代码规范要求）；

> 简单说：\n 是 “让代码能跑”，\t 是 “让代码好看”。

###### 4.1 寄存器赋值
将上面的源码分为两部分，`：：`前面的是操作，`：：`后面的是赋值。CPU会`先执行`后面的赋值操作:
如下： 具体的解释包含在注释中了。
```c
	: "i" ((short) (0x8000+(dpl<<13)+(type<<8))), \ //这里i表示一个立即数(instant)，即0x8000+(dpl<<13)+(type<<8),后续对应%0
	"o" (*((char *) (gate_addr))), \    //这里o表示输出output，是gate_addr(即对应中断门)的地址，后续对应%1,后面会把具体中断处理函数的低4字节地址写入这里
	"o" (*(4+(char *) (gate_addr))), \ //这里o表示输出output，是gate_addr前面4字节(即对应中断门的高4字节)的地址，后续对应%2，后面会把具体中断处理函数的高4字节写入这里
	"d" ((char *) (addr)),  //这里的d表示%edx寄存器，即将我们传入的addr参数送入%edx寄存器中，后续对应%3
    "a" (0x00080000))  //这里的a表示%eax寄存器，即将0x00080000送入%eax寄存器中，后续对应%4
```
显然，
- 我们执行了"d"后，%edx寄存器中就存放了我们传入的addr参数，即中断处理函数地址。假如总的地址是：`0x12345678`
那么此时，%edx=0x12345678
- 我们执行了"a"后，%eax寄存器中就存放了0x00080000，即代码段选择子。
那么此时，%eax=0x00080000

**%edx和%eax的值，这两点要牢记！**

###### 4.2 寄存器操作
接下来讲解上面的寄存器操作代码：
```c
__asm__ (
    "movw %%dx,%%ax\n\t" \
	"movw %0,%%dx\n\t" \
	"movl %%eax,%1\n\t" \
	"movl %%edx,%2" \
```
###### movw %%dx,%%ax
注意这里是2字节大小操作，即movw，所以，这是把edx中的低16位(即addr低16位)写入eax的低16位。因为之前的"a"已经写入了%eax的高16位，还没有写入低16位。所以这里写入刚刚弥补，不冲突。

> 此时 eax = 0x0008（代码段选择子） + addr 低 16 位。


此时 eax = `0x0008`（`代码段选择子`） + addr 低 16 位 → 刚好对应门描述符的前 4 字节（地址低 16 位 + 代码段选择子）。

>注意，这个段选择子的值0x8000对应的索引就是1,因为我们的gdt[1]就是代码段描述符。
图示也可以支撑：1000结尾的//

![段选择子](image-3.png)

>**这里，我们的中断描述符的前 4 字节填充好了！**

>时刻要牢记中断描述符图像：

![中断描述符图像](image.png)

###### movw %0,%%dx
%0 是输入参数，对应 `0x8000+(dpl<<13)+(type<<8)（标志位）；`
所以此处，是写入标志位到`edx的低16字节`。
而我们的`addr 高 16 位`自然保留在edx的高16字节中。这是`第一步"d"就保留的`，所以不动。

> 这里的标志位是：0x8000+(dpl<<13)+(type<<8)
对应中断描述符的第 3 字节（标志位）。
>时刻要牢记中断描述符图像：

![中断描述符图像](image.png)



>%0,1,2,3的语法解释
> 为啥对应着这个结果？GCC 内联汇编对输入参数的编号规则，所有输入 / 输出参数都会按出现**顺序被编号为 %0、%1、%2...**，这是**编译器的硬性规则**，和参数内容无关。

把标志位写入 edx 的`低 16 位（dx）`，此时 edx = 标志位 + addr 高 16 位（addr 指针的高 16 位自然保留）。
> 这里的中断参数是：_set_gate(&idt[n],15,3,addr)，所以type=15=>1111 dpl=3=>011,
> 这里的13是因为，dpl<<13是为了把dpl左移13位,dpl也刚刚是2位，0~4的大小
> 这里的8是因为，type<<8是为了把type左移8位，15代表是中断门



> 中断的类型：
![中断的类型](image-4.png)
|门描述符类型|	对应 type 数值	|二进制（4 位类型位）|	关键特点（补充理解）|
|-|-|-|-|
|Task Gate（任务门）	|5	|0101	|用于任务切换，Linux 0.11 几乎不用|
| Interrupt Gate（32 位中断门）	|14	|1110	|触发后自动关中断（IF=0），用于硬件中断|
| Trap Gate（32 位陷阱门）	|15	|1111	|触发后不关中断（IF=1），用于异常 / **系统调用**|

显然，这个中断门的架构是一致的：

![中断描述符图像](image.png)

> 而且，这里传入15的类型，也证明了，这就是系统调用！


所以，经过
```c
"movw %%dx,%%ax\n\t" \
"movw %0,%%dx\n\t" \
```
**这两步，%eax和%edx里面，存着的分别是中断描述符的前 4 字节（地址低 16 位 + 代码段选择子）和后 4 字节（ 地址高 16 位 +标志位）。**

那当前，下面的两部，就是把%eax和%edx里面的内容，写入对应的中断描述符地址。让其落实在内存层面，彻底生效！

###### movl %%eax,%1
上面已经做好了 eax 的赋值，即 eax = 0x0008（代码段选择子） + `addr 低 16 位`。现在将其写入对应的中断描述符地址。

> 把 eax 的 32 位值写入 %1（对应 `gate_addr 起始地址`），即完成`门描述符前 4 字节的写入`（地址低 16 位 + 代码段选择子）。

###### movl %%edx,%2
上面已经做好了 edx 的赋值，即 edx = `addr 高 16 位`+标志位  。现在将其写入对应的中断描述符地址。

>把 edx 的 32 位值写入 %2（对应 `gate_addr + 4 字节地址`），即完成门描述符后 `4 字节的写入`（标志位 + 地址高 16 位）。


====


那么具体的逻辑就到了fork的实现：
首先，我们的系统调用功能功能号是__NR_##name，对应在这里，就是__NR_fork，就是2。
2，将是系统调用表的索引，我们会调用这个索引处的函数：
