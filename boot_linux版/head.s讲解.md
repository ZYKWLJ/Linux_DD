# 一、作用
1. 重新设置保护模式下的GDT和IDT
2. 初始化页目录表、页表
3. 开启分页机制
4. 跳转到内核main函数

# 二、按作用逐行讲解head.s代码

## (一)分页模式的伏笔，开头定下页目录表、页表标签地址
```s
.text # 声明代码段（text section），告诉汇编器 / 链接器，接下来的内容属于可执行代码区域，而非数据区（.data）或未初始化数据区（.bss）。
.globl idt,gdt,pg_dir,tmp_floppy_area # 全局可见变量
pg_dir: # 预留的页目录表、页表地址标签。这里只是一个 “符号标签”，本身不分配内存
.globl startup_32 # 内核从实模式（16 位）切换到保护模式（32 位）后的第一个 32 位执行入口函数
```

## (二)设置内核堆栈指向+重新设置GDT、IDT

### 1.设置内核堆栈指向+重新设置GDT、IDT
```s
startup_32:
# 给数据段寄存器（DS/ES/FS/GS）加载保护模式的段选择子（0x10）
	movl $0x10,%eax # 0000 0000 0001 0000,索引为2,即数据段
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	mov %ax,%gs
	lss stack_start,%esp # 设置内核栈的栈指针（ESP）和栈段（SS），lss是 32 位汇编指令，全称 “Load SS and ESP”
	call setup_idt # 初始化中断描述符表（IDT）
	call setup_gdt # 重新初始化全局描述符表（GDT）(前面初始化过一次)

# 重新加载段寄存器（DS/ES/FS/GS），因为setup_gdt修改了各个段寄存器，需要重新加载
	movl $0x10,%eax		# reload all the segment registers
	mov %ax,%ds		# after changing gdt. CS was already
	mov %ax,%es		# reloaded in 'setup_gdt'
	mov %ax,%fs
	mov %ax,%gs
	lss stack_start,%esp # 再次设置栈指针：确保 GDT 更新后，栈的段（SS）和偏移（ESP）依然指向正确的内核栈空间（冗余操作，提升鲁棒性）。
```

注意，这里的`setup_idt`和`setup_gdt`定义如下：
### 2.setup_idt具体实现
构建 **256 个中断门描述符（IDT 的表项）**，每个描述符都指向**同一个默认中断处理函数ignore_int(我们称之为哑中断)**（忽略未处理的中断）；

```s
setup_idt:
	lea ignore_int,%edx
	movl $0x00080000,%eax
	movw %dx,%ax		/* selector = 0x0008 = cs */
	movw $0x8E00,%dx	/* interrupt gate - dpl=0, present */

	lea idt,%edi
	mov $256,%ecx

rp_sidt:
	movl %eax,(%edi)
	movl %edx,4(%edi)
	addl $8,%edi
	dec %ecx
	jne rp_sidt # 循环跳转的
	lidt idt_descr # 必备操作，加载IDT，确保CPU知道IDT的位置
	ret
```
对应的默认中断处理函数ignore_int的定义如下：

```s
ignore_int:
	pushl %eax
	pushl %ecx
	pushl %edx
	push %ds
	push %es
	push %fs
	movl $0x10,%eax
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	pushl $int_msg # 后续对应实现为`int_msg: .asciz "Unknown interrupt\n\r"`也即，哑中断，未知的。
	call printk # 打印统一的哑中断信息
	popl %eax
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret
```

而对应的`idt_descr`定义如下：
idt_descr是一个 6 字节的结构体，用于描述IDT的位置和大小。
如图：
```s
idt_descr:
	.word 256*8-1		# idt contains 256 entries，一共256个中断门描述符，每一个8字节
	.long idt           # idt基地址
.align 2
```
这里的`idt_descr`定义了256个中断门描述符，每个描述符的大小为8字节（64位），共256*8=2048字节。


### 3.setup_gdt具体实现
```s
setup_gdt:
	lgdt gdt_descr
	ret
```
而对应的`gdt_descr`定义如下：
gdt_descr是一个 6 字节的结构体，用于描述GDT的位置和大小。
如图：
```s
gdt_descr:
	.word 256*8-1		# so does gdt (not that that's any'，一共256个GDT描述符，每一个8字节
	.long gdt		# magic number, but it works for me :^)，gdt基地址
	.align 8
```

### 4.idt的具体内容：
```s
idt:	
    .fill 256,8,0		# idt is uninitialized
```

- .fill 256,8,0：这是汇编器的伪指令（伪操作），作用是填充内存：
- 第一个参数 256：表示要填充 256 个单元（对应 x86 架构下 256 个中断向量，0~255）；
- 第二个参数 8：表示每个单元占 8 字节（x86-64 架构下，IDT 的每个中断描述符是 8 字节；32 位下是 6 字节，但这里用 8 字节是为了对齐）；
- 第三个参数 0：表示填充的数值是 0（即初始状态下所有中断描述符都置空）；

### 5.gdt的具体内容：
```s
gdt:	
    .quad 0x0000000000000000	/* NULL descriptor ,这就是GDT的第一个描述符，必须为0*/
	.quad 0x00c09a0000000fff	/* 16Mb，定义一个内核级的可执行代码段，覆盖 16MB 内存空间（供内核代码执行使用） */
	.quad 0x00c0920000000fff	/* 16Mb，定义一个内核级的可读写数据段，同样覆盖 16MB 内存空间（供内核数据存储使用）。 */
	.quad 0x0000000000000000	/* TEMPORARY - don't use' */
	.fill 252,8,0			/* space for LDT's and TSS's etc  */
```
.quad：汇编伪指令，意为 “定义一个`8 字节（64 位）的数值`”（对应 x86-64 下的 `GDT 描述符格式`）


## (三)检查一下A20地址线是否真的开启
这里注意，原来真的可以使用数字作为地址标签标记！
```s
	xorl %eax,%eax # 把 EAX 寄存器清零（xorl %eax,%eax是汇编中高效清零寄存器的方式，比movl $0,%eax更快）。
# 检查 A20 地址线是否真的开启。
1:	incl %eax		# 定义一个局部标签（循环入口），为1b
	movl %eax,0x000000	# loop forever if it isn't'
	cmpl %eax,0x100000
	je 1b
```

## (四)跳转到after_page_tables执行，并在里面设置页表、跳转到main函数，进入真正的内核
### 0.页表的讲解
CPU提供的保护模式有**分段、分页**两种，也称为**段页式**内存管理机制。

- **分段模式**将进程映射到不同的线性地址空间，**分页模式**将进程映射到不同的物理地址空间。
#### (1)分段模式
就是前面说的，我们在段寄存器里面拿到段选择子，再根据段选择子中的索引，去GDT中查找对应的段描述符，在段描述符里面就可以得到段基地址。基地址相加偏移地址，得到的就是线性地址。

这里，CPU就会将不同的进程映射到不同的线性地址空间。在linux0.11里面是每一个进程64MB，那么64个进程就是2^(20+6+6)=4GB,刚好是32位，也就是段基地址的位数。所以CPU通过这样的机制隔离了不同进程的线性地址空间。


#### (2)分页模式
如果开启了分页模式，那么CPU会将上述的地址称为线性地址，并再次转换为物理地址。
具体来说，CPU会将线性地址分为`页目录索引`和`页表索引`,`页内偏移` 3部分。
![开启了分页模式后，线性地址被解释成3部分，`页目录索引`和`页表索引`,`页内偏移`](image-4.png)
- 页目录索引：高10位
- 页表索引：中10位
- 页内偏移：低12位

然后，CPU会根据`页目录索引`，去`页目录`中查找对应的`页表地址`。
再根据`页表索引`，去`页表`中查找对应的`物理页框地址`。
最后，将`页内偏移`与`物理页框地址`相加，得到最终的`物理地址`。


### 1.逆向思维，先把进入main函数的事情入栈做好了
这里其实有了很多逆向思维，设置页表的事情全部包含在after_page_tables之中了。

```s
	jmp after_page_tables
```
具体如下的定义：
```s
after_page_tables:
	pushl $0		# These are the parameters to main :-)
	pushl $0
	pushl $0
	pushl $L6		# return address for main, if it decides to.
	pushl $main     # 会由这里进入我们的main函数，但在此之前，我们要先执行完页表的设置，然后再由CPU自动取出栈顶的main定位标签，从而跳转到main函数之中去！当然这里的main函数，就是内核的main函数，C形式的，编译时其实会转化为汇编代码的，会生成main标签。这里充分利用了CPU之于堆栈取指令的功能，不愧是linus，奇淫技巧多得很。

	jmp setup_paging 

L6:             # 这里可以省略的，但是linus这么写了，就这样吧
	jmp L6		# main should never return here, but
				# just in case, we know what happens.
```

### 2.设置页表

再次给出设置页表`setup_paging`的定义：

这其实是初始化分页机制的核心汇编代码，也是从 **“分段式保护模式”** 切换到 **“分段 + 分页保护模式”** 的关键步骤。
![页表项、页表结构](image-5.png)
```s
# 分页相关的表（pg_dir、页表）必须按 4 字节对齐（因为每个表项占 4 字节），.align 2确保setup_paging函数的指令起始地址对齐，避免内存访问效率降低或错误。
# .align 2, GAS 汇编伪指令，要求后续代码 / 数据对齐到4 字节边界（2²=4）；
.align 2 
setup_paging:

# 需要清空5 个 4KB 页面（1 个页目录表 + 4 个页表），每个页面有 1024 个 4 字节表项，总共 5×1024=5120 个表项；
	movl $1024*5,%ecx		# ecx作为计数器，用于清空5个页面 
	xorl %eax,%eax
	xorl %edi,%edi			# pg_dir is at 0x000 ，这和前面呼应了
	cld;                    # 让字符串指令（如 stosl）按地址递增方向执行
    rep;                    # Repeat（重复执行后续指令），次数 = ECX 的值（5120 次）；
    stosl                   # 循环指令，Store String Long（32 位字符串存储）

# 配置页目录表的前 4 个表项，分别指向 pg0-pg3 这 4 个页表，并设置权限
# 7 的二进制是111，对应页目录项的属性位：位 0（P）=1（存在）、位 1（R/W）=1（可读写）、位 2（U/S）=1（用户可访问）
# 高 20 位 = pg0 的物理页框号（pg0 地址 >>12）；

	movl $pg0+7,pg_dir		# 第一个页表，权限设置
	movl $pg1+7,pg_dir+4	# 第二个页表
	movl $pg2+7,pg_dir+8	# 第三个页表
	movl $pg3+7,pg_dir+12	# 第四个页表
	movl $pg3+4092,%edi     # pg3 页表的最后一个表项的起始位置，将此地址存入 EDI，作为后续反向填充页表的起始地址。
	movl $0xfff007,%eax		
                            # 立即数0xfff007解析：
                            # 高 20 位：0x00fff,表示的是页框号，即页框号为0xfff，4095嘛，页框号和物理起始地址一一对应，具体来说需要左移12位才得到物理地址，左移 12 位后得到物理页起始地址0xfff000（即16MB-4KB）刚刚对应最后一页位置；
                            # 低 12 位：属性为，最后3位为7（属性位，存在 + 可读写 + 用户可访问）；
                            # 作用：EAX 保存最后一个页表项的值（映射 16MB-4KB 到 0xfff000），怎么映射的？就是硬件自动完成的两级查表过程，先根据页目录索引找到页表地址，再根据页表索引找到页框号，最后将页框号左移12位得到页物理地址，再与物理偏移地址相加，得到最终的物理地址。
	std

1:	stosl			/* fill pages backwards - more efficient :-) */
	subl $0x1000,%eax
	jge 1b
	cld
	xorl %eax,%eax		/* pg_dir is at 0x0000 */
	movl %eax,%cr3		/* cr3 - page directory start */
	movl %cr0,%eax
	orl $0x80000000,%eax
	movl %eax,%cr0		/* set paging (PG) bit */
	ret			/* this also flushes prefetch-queue */
```

这里的页表地址如下：

![设置完后的页表架构](image-6.png)

```s
    .org 0x1000 # 1*16^3=4096，代表了第一个页表的位置
    pg0:

    .org 0x2000 #从这里也可以看出来，每一个页表的项总大小是4096B，然后每一个页表项的大小是4B，所以每一个页表有1024个项，每个项4B，所以每个页表4096B。
    pg1:

    .org 0x3000
    pg2:

    .org 0x4000
    pg3:

    .org 0x5000
```

## (五)最终的内存布局
最终的内存布局如图，最开始就是页目录表、页表的内容，后面再才是system剩下的部分。
其次，CPU中的CR3寄存器指向的页目录表，gtdr执行gdt表，idtr指向dtr表。
最终CPU从栈顶去除main的地址，进入内核。
![最终的内存布局如图，最开始就是页目录表、页表的内容](image-7.png)