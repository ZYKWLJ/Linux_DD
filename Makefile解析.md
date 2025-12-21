# 一、作用：
核心作用是把 Linux 早期内核的**各类源码（汇编、C 语言）编译、链接成可执行文件**，再打包成能在 x86 架构上启动的**软盘镜像（Boot.img）**。

# 二、逐行解释：
## (一)第一部分，核心变量定义
```makefile
# 虚拟行1: 注释 - 说明RAMDISK（内存盘）的配置方式
# if you want the ram-disk device, define this to be the
# size in blocks.
#
# 虚拟行4: 变量定义 - 内存盘配置（当前注释掉，未启用）
RAMDISK =  #-DRAMDISK=512

# 虚拟行6-7: 注释 - 老式86架构汇编/链接工具（已被NASM替代）
#AS86	=as86 -0 -a
#LD86	=ld86 -0

# 虚拟行9: 变量定义 - 头文件路径（boot目录）
INC=boot
# 虚拟行10: 变量定义 - 16位汇编器（用NASM替代老式as86）
AS86 = nasm 
# 虚拟行11: 变量定义 - 反汇编工具（用于调试汇编代码）
DASM = ndisasm

# 虚拟行13: 变量定义 - 32位汇编器（GNU as）
AS	=as
# 虚拟行14: 变量定义 - 链接器（GNU ld）
LD	=ld
# 虚拟行15: 注释 - 老式链接参数（ELF格式）
#LDFLAGS	= -m elf_i386 -Ttext 0 -e startup_32
# 虚拟行16: 变量定义 - 链接器参数（核心）
LDFLAGS	  = -m i386pe   -Ttext 0 -e startup_32 -s -M --image-base 0x0000 
# 解释：
# 链接参数（内核定制）：
# ✅ -m i386pe：生成 32 位 Windows PE 格式（适配 MinGW 交叉编译）；
# ✅ -Ttext 0：强制代码段起始地址为 0（内核最终运行在物理地址 0x100000，链接时先设为 0，启动时重定位）；
# ✅ -e startup_32：指定程序入口为 head.o 中的 startup_32 函数（内核不依赖 main 作为入口）；
# ✅ -s：剥离符号表（减小文件体积，不影响运行）；
# ✅ -M：输出链接映射信息（符号地址、段布局）；
# ✅ --image-base 0x0000：PE 镜像基地址设为 0（配合 -Ttext 0）；

# 虚拟行17: 变量定义 - C编译器（GCC）+ 架构 + 内存盘参数
CC	=gcc -march=i386 $(RAMDISK)
# 解释：
# -march=i386：针对32位x86架构编译
# $(RAMDISK)：引用上面定义的RAMDISK变量（当前为空）

# 虚拟行18: 变量定义 - C编译参数（内核编译核心参数）
CFLAGS	=-Wall -O2 -fomit-frame-pointer -fno-builtin
# 解释：
# -Wall：开启所有警告
# -O2：2级优化（平衡性能和编译速度）
# -fomit-frame-pointer：省略帧指针（节省寄存器，提升性能）
# -fno-builtin：禁用GCC内置函数（内核需自定义）

# 虚拟行20: 变量定义 - C预处理器（CPP）参数
CPP	=cpp -nostdinc -Iinclude
# 解释：
# -nostdinc：禁用系统标准头文件（内核用自己的include目录）
# -Iinclude：添加头文件搜索路径（当前目录的include）

# 虚拟行22-24: 注释 - 根设备配置说明
# ROOT_DEV specifies the default root-device when making the image.
# This can be either FLOPPY, /dev/xxxx or empty, in which case the
# default of /dev/hd6 is used by 'build'.
#
# 虚拟行25: 变量定义 - 根设备（当前注释掉，默认用hd6）
ROOT_DEV= #FLOPPY 
```

## (二)第二部分，源码文件归类
这就是Makefile的好处，一目了然！
```makefile
# 虚拟行27: 变量定义 - 块设备驱动源码列表（硬盘/软盘/内存盘），4个
BLK_DRV_SRC  = kernel/blk_drv/ll_rw_blk.c kernel/blk_drv/floppy.c \
                kernel/blk_drv/hd.c kernel/blk_drv/ramdisk.c
# 解释：\ 是换行续行符，把多行合并为一个变量

# 虚拟行30: 变量定义 - 字符设备驱动源码列表（终端/键盘/串口），6个
CHR_DRV_SRC  = kernel/chr_drv/tty_io.c kernel/chr_drv/console.c kernel/chr_drv/keyboard.s \
        kernel/chr_drv/serial.c kernel/chr_drv/rs_io.s \
	      kernel/chr_drv/tty_ioctl.s

# 虚拟行34: 变量定义 - 内核核心源码列表（调度/系统调用/进程管理），12个
KERNEL_SRC  = kernel/sched.c kernel/system_call.s kernel/traps.s kernel/asm.s kernel/fork.c \
	kernel/panic.c kernel/printk.c kernel/vsprintf.c kernel/sys.c kernel/exit.c \
	kernel/signal.c kernel/mktime.c

# 虚拟行38: 变量定义 - 内存管理源码列表，2个
MM_SRC	= mm/memory.c mm/page.s

# 虚拟行40: 变量定义 - 文件系统源码列表，17个
FS_SRC=	fs/open.c fs/read_write.c fs/inode.c fs/file_table.c fs/buffer.c fs/super.c \
	fs/block_dev.c fs/char_dev.c fs/file_dev.c fs/stat.c fs/exec.c fs/pipe.c fs/namei.c \
	fs/bitmap.c fs/fcntl.c fs/ioctl.c fs/truncate.c

# 虚拟行45: 变量定义 - 内核库函数源码列表（字符串/内存/系统调用封装）,12个
LIB_SRC  = lib/ctype.c lib/_exit.c lib/open.c lib/close.c lib/errno.c lib/write.c lib/dup.c lib/setsid.c \
	lib/execve.c lib/wait.c lib/string.c lib/malloc.c
```

## (三)第三部分：归档文件定义
各司其职，很好！
```makefile
# 虚拟行49: 变量定义 - 内核核心归档文件（多个.o合并为.o）
ARCHIVES=kernel/kernel.o mm/mm.o fs/fs.o
# 注释：原注释是把KERNEL_OBJS拆分开，当前简化为直接引用.o

# 虚拟行51: 变量定义 - 驱动归档文件（.a是静态库）
DRIVERS =kernel/blk_drv/blk_drv.a kernel/chr_drv/chr_drv.a

# 虚拟行52: 变量定义 - 数学协处理器模拟库
MATH	=kernel/math/math.a
# 虚拟行53: 变量定义 - 内核库函数归档文件
LIBS	=lib/lib.a
```

## (四)第四部分：后缀规则（编译规则）
这里的后缀规则很清晰，分别是：
- .c.s：C源码编译为汇编代码
- .s.o：汇编源码编译为目标文件
- .c.o：C源码编译为目标文件

下次，自己可以这么搞，很美。
具体请查看这篇文章：![7.隐式规则(后缀规则)_解耦在Makefile中的使用](/boot_linux版/Makefile基础知识讲解.md/)

```makefile
# 虚拟行55: 后缀规则 - .c文件转.s文件（C源码编译为汇编代码）
.c.s:
	$(CC) $(CFLAGS) \
	-nostdinc -Iinclude -S -o $*.s $<
# 路径细节：路径细节：include 是相对路径，指执行 make 命令时的当前目录（内核源码根目录）下的 include 文件夹，而非系统路径。

# 解释：
# $*: 匹配的文件名（无后缀），如main.c → main
# $<: 依赖文件（源文件），如main.c
# -S: 只编译为汇编代码，不生成目标文件
# \: 命令换行续行

# 虚拟行58: 后缀规则 - .s文件转.o文件（汇编源码编译为目标文件）
.s.o:
	$(AS)  -o $*.o $<

# 虚拟行60: 后缀规则 - .c文件转.o文件（C源码编译为目标文件）
.c.o:
	$(CC) $(CFLAGS) \
	-nostdinc -Iinclude -c -o $*.o $<
# 解释：-c: 只编译不链接，生成.o目标文件
```
注意这里的`-nostdinc -Iinclude`:
```makefile
# -nostdinc：禁用系统标准头文件目录,相当于告诉编译器「别去系统默认的 “公共头文件仓库” 找文件，我有自己的专属仓库」。
# -Iinclude：添加自定义头文件搜索路径，编译器会优先在 -I 指定的路径下查找头文件（比如内核代码中 #include <linux/sched.h> 会去 ./include/linux/ 找）；结合 -nostdinc 后，编译器只会在 -I 指定的路径（以及代码所在目录）找头文件，彻底隔离系统标准头文件，保证内核编译的独立性。
```


## (五)第五部分：核心目标（生成 Boot.img，即包含整个操作系统内容的引导镜像）
```makefile
# 虚拟行63: 伪目标 - all（默认目标，执行make时优先执行）
all:	Boot.img

# 虚拟行65: 变量定义 - 内核二进制文件路径
KERNEL_FILE = tools/system.bin

# 虚拟行67: 规则 - 生成system.bin（PE格式转纯二进制）
tools/system.bin : tools/system.exe 
	tools/Trans.exe tools/System.exe tools/system.bin
# 解释：Trans.exe是自定义工具，把Windows PE格式的system.exe转为纯二进制的system.bin（内核可执行镜像）
	
# 虚拟行70: 核心规则 - 生成Boot.img（最终的引导镜像）

Boot.img: boot/bootsect.bin boot/setup.bin $(KERNEL_FILE) tools/build.exe Makefile
	tools\build.exe boot/bootsect.bin boot/setup.bin $(KERNEL_FILE) $(ROOT_DEV)

# 当执行 make Boot.img 时，先检查所有依赖文件是否存在且最新，若满足则调用 tools/build.exe 工具，将引导扇区、设置程序、内核二进制文件打包为可直接写入软盘的引导镜像 Boot.img

# 引导镜像是整个操作系统的全体打包。
# 一个很好的比喻就是把整个内核比喻成在屋子里玩耍，那么，bootsect段是钥匙，setup段是进屋后的放下书包，换好鞋子等准备工作，system才是畅享整个OS的环节。


#	rm tools/kernel -f  # 注释：清理临时文件
#	sync  # 注释：Linux同步磁盘缓存，Windows无需
#	del tools\kernel  # 注释：Windows清理临时文件

# 解释：
# build.exe是自定义工具，作用是：
# 1. 把bootsect.bin（引导扇区，512字节）、setup.bin（设置程序）、system.bin（内核）拼接
# 2. 根据ROOT_DEV设置根设备，生成可引导的Boot.img镜像

# 虚拟行78: 注释目标 - disk（把镜像写入软盘，早期Linux启动方式）
disk: Image
	dd bs=8192 if=Image of=/dev/fd0
# 解释：dd是Linux命令，把Image镜像写入软盘设备/dev/fd0（Windows无此设备），得到一张可以启动linux的软盘。
```

## (六)第六部分：工具编译 + 汇编文件编译
```makefile
# 虚拟行82: 规则 - 编译build.exe（镜像打包工具）
# 这就是生成的build.exe!
tools/build.exe: tools/build.c
	$(CC) $(CFLAGS)	-o tools/build tools/build.c 

# 虚拟行84: 规则 - 编译head.o（内核头部汇编文件，32位入口）
boot/head.o: boot/head.s
	gcc -I./include -traditional -c boot/head.s -o boot/head.o
# 解释：
# -traditional：兼容老式C语法（内核源码用老式语法）
# head.s是内核32位启动的入口文件，必须单独编译

# 虚拟行88: 规则 - 生成system.exe（内核可执行文件）
tools/system.exe:	boot/head.o init/main.o \
		$(ARCHIVES) $(DRIVERS) $(MATH) $(LIBS) # 依赖定义
# 链接输入文件，和依赖的定义顺序一样，不能乱！因为链接器按「从左到右」解析符号
	$(LD) $(LDFLAGS) \ 
    boot/head.o init/main.o \
	$(ARCHIVES) \
	$(DRIVERS) \
	$(MATH) \
	$(LIBS) \
	-o tools/system.exe >system.map
# 链接的输入文件：
# 完全复用规则头的依赖列表，顺序绝对不能乱！链接器按「从左到右」解析符号，先加载 head.o（入口），再加载 main.o（初始
# 化），最后加载库（驱动 / 基础库），确保底层符号能被上层调用（比如 main.o 调用 fs.o 的文件系统函数）；

# 牛逼科斯拉！这里，把全部都拼接起来了！
# 解释：
# 链接所有内核模块：头部(head.o) + 主函数(main.o) + 核心模块 + 驱动 + 库
# >system.map：把链接映射信息输出到system.map（调试用）
# 原注释：nm命令过滤符号表生成System.map，当前简化为直接输出
```

## (七)第七部分：子目录编译规则
```makefile
# 虚拟行92: 规则 - 生成数学库math.a
kernel/math/math.a:kernel/math/math_emulate.o
#	(cd kernel/math ; make)  # Linux下进入子目录执行make,;表示多条命令一起执行。
	(cd kernel/math & make)  # Windows下&替代;，执行子目录Makefile
	
# 虚拟行95: 规则 - 生成块设备驱动库blk_drv.a
kernel/blk_drv/blk_drv.a:$(BLK_DRV_SRC)
#	(cd kernel/blk_drv ; make)
	(cd kernel/blk_drv & make)
	
# 虚拟行98: 规则 - 生成字符设备驱动库chr_drv.a
kernel/chr_drv/chr_drv.a:$(CHR_DRV_SRC)
#	(cd kernel/chr_drv ; make)
	(cd kernel/chr_drv & make)
	
# 虚拟行101: 规则 - 生成内核核心kernel.o
kernel/kernel.o:$(KERNEL_SRC)
#	(cd kernel; make)
	(cd kernel & make)

# 虚拟行103: 规则 - 生成内存管理mm.o
mm/mm.o:$(MM_SRC)
#	(cd mm; make)
	(cd mm & make)
	
# 虚拟行105: 规则 - 生成文件系统fs.o
fs/fs.o:$(FS_SRC)
#	(cd fs; make)
	(cd fs & make)
	
# 虚拟行107: 规则 - 生成库函数lib.a
lib/lib.a:$(LIB_SRC)
	(cd lib & make)
```

## (八)第八部分：引导程序编译（bootsect/setup）
这个竟然是在下面才编译的。
```makefile
# 虚拟行109: 规则 - 生成setup.bin（设置程序，16位汇编）
boot/setup.bin: boot/setup.asm 
	$(AS86) -I"$(INC)"\ -o boot/setup.bin boot/setup.asm
	 $(DASM) -b 16 boot/setup.bin >boot/setup.disasm
#	$(LD86) -s -o boot/setup boot/setup.o  # 注释：老式链接方式
# 解释：
# -I"$(INC)"：添加头文件路径（boot目录）
# -b 16：反汇编为16位代码，生成disasm文件（调试用）

# 虚拟行114: 规则 - 生成bootsect.bin（引导扇区，512字节）
boot/bootsect.bin:	boot/bootsect.asm 
	$(AS86) -I"$(INC)"\ -o boot/bootsect.bin boot/bootsect.asm
	$(DASM) -b 16 boot/bootsect.bin >boot/bootsect.disasm
#	$(LD86) -s -o boot/bootsect boot/bootsect.o  # 注释：老式链接方式
```

## (九)第九部分：临时文件生成（注释）
```makefile
# 虚拟行119: 注释规则 - 生成tmp.s（拼接bootsect.s和系统大小）
tmp.s:	boot/bootsect.s tools/system
#	(echo -n "SYSSIZE = (";ls -l tools/system | grep system \
#		| cut -c25-31 | tr '\012' ' '; echo "+ 15 ) / 16") > tmp.s
	cat boot/bootsect.s >> tmp.s
# 解释：原逻辑是计算内核大小并写入tmp.s，当前简化为直接拼接
```

## (十)第十部分：清理（clean/backup/dep）

```makefile
# 虚拟行124: 伪目标 - clean（清理编译产物）
clean:
#	rm -f Image System.map tmp_make core boot/bootsect boot/setup  # Linux清理命令
#	rm -f init/*.o tools/system tools/build boot/*.o
#	(cd mm;make clean)
#	(cd fs;make clean)
#	(cd kernel;make clean)
#	(cd lib;make clean)
	@del /S /Q *.a *.o system.map tools\system.exe  # Windows清理命令
#	del Image System.map tmp_make core boot\bootsect boot\setup
#	del init\*.o tools\system tools\build boot\*.o
#	(cd mm & make clean)
#	(cd fs & make clean)
#	(cd kernel & make clean)
#	(cd lib &make clean)
# 解释：
# @：执行命令时不打印命令本身
# /S：递归删除子目录
# /Q：静默删除（无需确认）

# 虚拟行138: 伪目标 - backup（备份源码）
backup: clean
	(cd .. ; tar cf - linux | compress16 - > backup.Z)
	sync
```

## (十一)第十一部分：依赖(dep)
```makefile
# 虚拟行141: 伪目标 - dep（生成依赖文件）
dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in init/*.c;do echo -n "init/";$(CPP) -M $$i;done) >> tmp_make
#	cp tmp_make Makefile  # Linux复制命令
#	(cd fs; make dep)
#	(cd kernel; make dep)
#	(cd mm; make dep)
	
	copy tmp_make Makefile  # Windows复制命令
	(cd fs & make dep)
	(cd kernel & make dep)
	(cd mm & make dep)

# 虚拟行150: 注释 - 依赖关系（自动生成的）
### Dependencies:
init/main.o: init/main.c include/unistd.h include/sys/stat.h \
  include/sys/types.h include/sys/times.h include/sys/utsname.h \
  include/utime.h include/time.h include/linux/tty.h include/termios.h \
  include/linux/sched.h include/linux/head.h include/linux/fs.h \
  include/linux/mm.h include/signal.h include/asm/system.h \
  include/asm/io.h include/stddef.h include/stdarg.h include/fcntl.h
# 解释：这是dep目标生成的依赖关系，说明main.o依赖哪些头文件
```

# 三、总结
- 这份 Makefile 是Linux 0.11 内核的 Windows 交叉编译脚本，核心是将 16 位引导程序、32 位内核源码编译链接为可引导的磁盘镜像（Boot.img）；

- 关键语法：变量定义（如 CC/CFLAGS）、后缀规则（.c.o/.s.o）、目标-依赖规则（如 Boot.img 依赖 bootsect.bin）、特殊变量（\(</\)*）；

- 核心流程：编译引导程序（bootsect/setup）→ 编译内核各模块 → 链接为 system.exe → 转换为 system.bin → 打包为 Boot.img。

每一行的设计都围绕`「内核编译」`展开，兼顾了老式 `Linux 编译工具`和 `Windows 交叉编译`的`适配`，是windows下借助`bochs`实现linux调试的典型实现。
