请先看本目录下的基础讲解：
> [Makefile基础知识讲解](Makefile基础知识讲解.md)（点击跳转至上级目录的说明文档）

# 一、作用：
1. 将bootsect.s、setup.s链接成二进制文件 
2. 将head.s编译成可重定位目标文件.o

# 一、源码：
```makefile
include ../Makefile.header

LDFLAGS	+= -Ttext 0

all: bootsect setup # 定义默认执行目标(组合目标)

bootsect: bootsect.s
	@$(AS) -o bootsect.o bootsect.s
	@$(LD) $(LDFLAGS) -o bootsect bootsect.o
	@$(OBJCOPY) -R .pdr -R .comment -R.note -S -O binary bootsect


setup: setup.s
	@$(AS) -o setup.o setup.s
	@$(LD) $(LDFLAGS) -o setup setup.o
	@$(OBJCOPY) -R .pdr -R .comment -R.note -S -O binary setup

head.o: head.s
	@$(AS) -o head.o head.s

clean:
	@rm -f bootsect bootsect.o setup setup.o head.o
```

# 二、解析：
## (一)包含外部Makefile.header
Makefile.header就是类似于定义了多个Makefile文件的全局变量。可以多个Makefile文件通用。

### (1)作用：

引入**上级目录**的Makefile.header文件；
关于此文件的讲解，请点击此处：![Linux0.11最外层的Makefile.header讲解](../makefile.header)

### (2)核心含义：

**复用通用配置**：该头文件会定义编译工具变量（**AS= 汇编器、LD= 链接器、OBJCOPY= 目标文件拷贝工具）、编译选项**等，**避免重复定义；**
类似 C 语言的#include，把Makefile.header的内容 **“粘贴” 到当前位置；**

## (二) LDFLAGS	+= -Ttext 0

### 1.作用：给链接器（LD）追加链接选项-Ttext 0；
### 2.核心含义：

- LDFLAGS：链接器的参数变量，+=表示在原有基础上`追加`（而非覆盖）；
- -Ttext 0：指定程序的`文本段（代码段）`加载到`内存地址 0 处`；

内核启动代码（bootsect/setup）运行在实模式，BIOS 加载引导扇区到 0x7c00，`但链接时先定位到 0`，后续`通过汇编代码调整地址`；
### 3.底层逻辑：
汇编代码编译后生成的目标文件`默认有 ELF 头`，链接时指定-Ttext 0`让代码段从 0 地址`开始，便于后续转换为纯二进制时地址对齐。

## (三)纯代码讲解
![需要结合上一层目录的makefile.header文件](../makefile.header)


```makefile
# 这里需要结合上一层目录的makefile.header文件，定义了AS、LD、OBJCOPY等变量
# AS= as --32、LD	= ld、OBJCOPY= objcopy
# $()在makefile中，是等价替换的意思
all: bootsect setup # 定义默认执行目标(组合目标),即make时，就执行这个

bootsect: bootsect.s
	@$(AS) -o bootsect.o bootsect.s   
	@$(LD) $(LDFLAGS) -o bootsect bootsect.o  
# 把链接后的 bootsect 可执行文件（ELF 格式）转换成纯二进制文件，同时删除所有无关的辅助信息，保证生成的文件是引导扇区所需的 “干净” 二进制（512 字节）
# objcopy，GNU binutils 中的二进制文件转换工具，用于复制 / 转换目标文件格式
# -R .pdr，关键参数：-R = --remove-section，删除文件中的 .pdr 段
# -R .comment，删除文件中的 .comment 段
# -R .note，删除文件中的 .note 段
# -S，关键参数：-S = --strip-all，删除所有符号表和调试信息
# -O binary，指定输出格式为二进制文件

# .pdr、.comment、.note 都属于「辅助段」—— 不参与程序运行，只用于编译 / 调试 / 备注，对引导扇区来说完全没用。
# .pdr 段，早期 GNU 编译器（gcc）为了支持调试、异常处理生成的辅助信息，记录函数的入口地址、参数信息、栈帧结构等
# .comment 段，注释段，当然删除
# .note 段，备注段，当然删除

	@$(OBJCOPY) -R .pdr -R .comment -R.note -S -O binary bootsect #OBJCOPY= objcopy

setup: setup.s
	@$(AS) -o setup.o setup.s
	@$(LD) $(LDFLAGS) -o setup setup.o
	@$(OBJCOPY) -R .pdr -R .comment -R.note -S -O binary setup

# 很奇怪，这里bootsect和setup都生成了二进制文件，而head.o只是一个目标文件，这是为什么？因为head.s后面要和system连在一起生成一个二进制文件。
head.o: head.s
	@$(AS) -o head.o head.s

clean:
	@rm -f bootsect bootsect.o setup setup.o head.o
```

