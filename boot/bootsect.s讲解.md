小红书视频：

# 一、作用：

### 1.作用：

1.将由bios加载到0x7c00的bootsect引导段二进制数据块复制到内存0x9000
2.立马跳转到0x9000处，并且将ds、ss、es都设置为0x9000，sp设置为0xff00，即将堆栈设置为0x9000:0xff00，即0x9ff00
3.将设置段setup二进制数据块加载到内存0x9020
4.将系统段system二进制数据块加载到内存0x10000

# 二、按功能逐行讲解代码

## (一)正式代码执行前的一些赋值


### 2.按功能逐行讲解代码
```s
# 开始执行前，boosect模块已经被bios加载到地址0x7c00处了！

    .code16 # 告诉汇编器， 接下来以16 位实模式 的 x86 指令集编译。背景：CPU 上电后默认进入实模式（8086 兼容模式），只能访问 1MB 内存，且段地址 + 偏移的寻址方式是 段地址*16 + 偏移，因此必须显式指定 16 位模式。
    .equ SYSSIZE, 0x3000 # 系统段system二进制数据块占用的内存空间段的大小，为0x3000字节，则换算为地址大小为0x30000,即196KB,足够Linux0.11了
	.global _start, begtext, begdata, begbss, endtext, enddata, endbss #将这些符号声明为全局可见，供链接器（ld）或其他模块（如 setup.s、内核）引用。_start：程序入口点（链接器会将此符号作为代码执行的起始位置）；begtext/endtext：代码段（.text）的起始 / 结束地址；begdata/enddata：数据段（.data）的起始 / 结束地址；begbss/endbss：未初始化数据段（.bss）的起始 / 结束地址；
	.text   #.text：代码段（存放执行指令，只读）；
	begtext:
	.data   #.data：数据段（存放已初始化的全局变量）；
	begdata:
	.bss    #.bss：未初始化数据段（存放未初始化的全局变量，运行时会被清零）；
	begbss:
	.text   #最后再次写 .text：将后续代码切回代码段（因为前面切到了.bss 段），保证启动指令在代码段执行。

# .equ：汇编伪指令，用于定义常量（类似 C 语言的 #define），后续代码可直接使用这些常量名；

	.equ SETUPLEN, 4		#  setup.s编译而成的数据所占扇区数，为4
	.equ BOOTSEG, 0x07c0		# 引导区初始段位置，0x07c0
	.equ INITSEG, 0x9000		# 把引导区移动到0x9000段地址处，防止被后续要加载的内容覆盖。linus的原话是，别挡路！
	.equ SETUPSEG, 0x9020		# setup.s编译而成的数据所在段位置，0x9020
	.equ SYSSEG, 0x1000		# 系统段system初始段位置，0x1000
	.equ ENDSEG, SYSSEG + SYSSIZE	# 系统段system二进制数据块结束位置，总共加载的大小196KB。远远足够我们的system数据大小了。

	.equ ROOT_DEV, 0x301 #根文件系统所在的设备号（0x301 对应第一个 IDE 硬盘的第一个分区）
	ljmp    $BOOTSEG, $_start # 执行远跳转（long jump），同时设置 CS 段寄存器和 IP 指令指针寄存器，进入真正的启动逻辑。这里就是跳到bootsect自己复制自己的位置。前面说了，这里是因为担心自己挡路，被后面的system模块覆盖，所以复制，所以这里就跳过去了！
```

## (二)_start处开始执行，复制0x7c00地址处的512B到0x9000处
这就是汇编固定执行的复制操作，没啥好说的
```s
_start:
	mov	$BOOTSEG, %ax	#将ds段寄存器设置为0x7C00，即引导区所在的段地址
	mov	%ax, %ds
	mov	$INITSEG, %ax	#将es段寄存器设置为0x9000，即引导区移动到的段地址
	mov	%ax, %es
	mov	$256, %cx		#设置移动计数值256字
	sub	%si, %si		#源地址	ds:si = 0x07C0:0x0000
	sub	%di, %di		#目标地址 es:si = 0x9000:0x0000
	rep					#重复执行并递减cx的值
	movsw				#从内存[si]处移动cx个字到[di]处
```

## (三)初始化内存架构，将ds、es、ss赋值为cs的值，即0x9000,并且将sp赋值为0xFF00
```s
#x86 汇编的远跳转（段间跳转）指令，同时修改 CPU 的 CS（代码段寄存器）和 IP（指令指针寄存器），实现跨段的代码跳转。即 “远跳转”，区别于只修改 IP 的短跳转 / 近跳转（jmp），远跳转必须指定段地址和偏移地址。所以之类的$INITSEG是段地址，$go是偏移地址。标志着CPU正式跳转到bootsect引导区复制的地方。

ljmp	$INITSEG, $go	

# 这里就是bootsect引导区复制的地方，在这里，我们开始执行内存初始化架构，说白了就是设置好堆栈段、数据段、拓展段。那么对应段式管理，当然就是操作这些段寄存器了！

# 另外，注意AT&T的写法，是符合人的从左到右的认知的^_^
go:	mov	%cs, %ax		#将ds，es，ss都设置成移动后代码所在的段处(0x9000)
	mov	%ax, %ds
	mov	%ax, %es
# put stack at 0x9ff00.
	mov	%ax, %ss
	mov	$0xFF00, %sp		# arbitrary value >>512
```

## (四)从软盘加载setup数据模块到0x90200内存处，共4扇区

```s
load_setup:                 # 标签：加载setup程序的入口
    mov $0x0000, %dx        # dx寄存器：指定磁盘驱动器和磁头
                            # dh = 0 (磁头0), dl = 0 (驱动器0，即第一个软盘)
    mov $0x0002, %cx        # cx寄存器：指定扇区和磁道
                            # ch = 0 (磁道0), cl = 2 (扇区2)；注：cx格式是高8位=磁道，低8位=扇区（低6位）
                            # 注释中“一共加载两块扇区”是最终要加载SETUPLEN个扇区，此处cx仅指定起始扇区
    mov $0x0200, %bx        # bx寄存器：指定数据加载到内存的偏移地址
                            # INITSEG（0x9000）是段地址，偏移0x200，最终物理地址=0x9000*16+0x200=0x90200
    .equ AX, 0x0200+SETUPLEN # 定义常量AX：0x0200（读扇区功能号） + SETUPLEN（要读取的扇区数）
                            # 0x0200的高8位是功能号（0x02），低8位是扇区数（SETUPLEN）
    mov $AX, %ax            # ax寄存器：高8位=0x02（BIOS 0x13中断的读扇区功能），低8位=SETUPLEN（读取的扇区数）
    int $0x13               # 触发BIOS 0x13中断：执行磁盘读操作，将指定扇区加载到指定内存地址
    jnc ok_load_setup       # 检查CF（进位标志）：CF=0（无进位）表示读取成功，跳转到ok_load_setup
                            # CF=1表示读取失败，执行后续重置磁盘的逻辑
    mov $0x0000, %dx        # 重置dx为0（驱动器0，磁头0）
    mov $0x0000, %ax        # ax=0x0000：BIOS 0x13中断的0号功能（重置磁盘控制器）
    int $0x13               # 触发0x13中断：重置磁盘控制器，恢复磁盘初始状态
    jmp load_setup          # 跳回load_setup标签，重新尝试读取扇区（循环直到成功）
```

## (五)完成了setup的加载后，接下来加载system，所以显示"Loading system ..."字样
```s
ok_load_setup:
# 第一部分：获取磁盘驱动器参数（每磁道扇区数）
# 这一部分代码的唯一目标是：问 BIOS“软盘驱动器 0（A 盘）的每一个磁道里有多少个扇区”，然后把这个数字记下来，给后面读取磁盘数据用。
# Get disk drive parameters, specifically nr of sectors/track

	mov	$0x00, %dl        # DL寄存器：指定要查询的驱动器号，0x00表示第一个软盘驱动器(A:)
	mov	$0x0800, %ax      # AH=0x08（功能号），AL=0x00；BIOS 0x13中断的0x08号功能是获取驱动器参数
	int	$0x13             # 调用BIOS 0x13中断，执行"获取驱动器参数"功能
	                        # 中断返回后：CX寄存器低6位=每磁道扇区数，CH=磁道号低8位，CL高2位=磁道号高2位
	mov	$0x00, %ch        # 将CH清零（只保留CL中的每磁道扇区数）
	#seg cs                 # 注释的伪指令，作用是指定后续操作的段寄存器为代码段CS
	mov	%cx, %cs:sectors+0 # 将CX的值（仅CL有效，每磁道扇区数）存入CS段的sectors变量，那么这个变量在哪里呢？在最后面的收尾(还可以这么玩，牛)
	mov	$INITSEG, %ax     # 将INITSEG（初始化段地址，如0x9000）加载到AX
	mov	%ax, %es          # 将AX的值赋给ES段寄存器，重置ES为初始化段

# 第二部分：打印提示信息
# 这一部分代码的唯一目标是：打印提示信息"IceCityOS is booting ..."到屏幕上。
	mov	$0x03, %ah        # AH=0x03，BIOS 0x10中断的0x03号功能：读取光标位置
	xor	%bh, %bh          # BH=0，指定操作的显示页为第0页（默认显示页）
	int	$0x10             # 调用BIOS 0x10中断，读取光标位置（返回后：DH=行号，DL=列号）
	
	mov	$30, %cx          # CX=30，指定要打印的字符串长度（msg1的字符数，这里也不是30啊，咋搞的？）
	mov	$0x0007, %bx      # BX=0x0007：BL=0x07（字符属性：黑底白字，正常显示），BH=0（显示页0）
	#lea	msg1, %bp        # 注释的指令：取msg1的有效地址到BP（等价于mov $msg1, %bp）
	mov     $msg1, %bp       # BP寄存器指向要打印的字符串msg1的起始地址
	mov	$0x1301, %ax      # AH=0x13（BIOS 0x10中断的0x13号功能：打印字符串），AL=0x01（打印后移动光标）
	int	$0x10             # 调用BIOS 0x10中断，打印msg1字符串到屏幕
```
## (六)从软盘第二扇区加载system数据模块到0x10000内存处
核心功能：先加载系统核心到指定内存段，再根据之前获取的**软盘扇区数**自动识别**根设备（root_dev）类型**
```s
	mov	$SYSSEG, %ax
	mov	%ax, %es		# segment of 0x010000
	call	read_it # 加载system模块
	call	kill_motor # 给软盘断电，因为我们Linux0.11是从软盘启动的，现在软盘已加载完毕，不需要继续保持电机运行，加载完了后，os就跑在内存里面了，暂时不需要软盘了
```
对应的封装函数如下：
### read_it函数
```s
read_it:
	mov	%es, %ax
	test	$0x0fff, %ax
die:	jne 	die			# es must be at 64kB boundary
	xor 	%bx, %bx		# bx is starting address within segment
rp_read:
	mov 	%es, %ax
 	cmp 	$ENDSEG, %ax		# have we loaded all yet?
	jb	ok1_read
	ret
ok1_read:
	#seg cs
	mov	%cs:sectors+0, %ax
	sub	sread, %ax
	mov	%ax, %cx
	shl	$9, %cx
	add	%bx, %cx
	jnc 	ok2_read
	je 	ok2_read
	xor 	%ax, %ax
	sub 	%bx, %ax
	shr 	$9, %ax
ok2_read:
	call 	read_track
	mov 	%ax, %cx
	add 	sread, %ax
	#seg cs
	cmp 	%cs:sectors+0, %ax
	jne 	ok3_read
	mov 	$1, %ax
	sub 	head, %ax
	jne 	ok4_read
	incw    track 
ok4_read:
	mov	%ax, head
	xor	%ax, %ax
ok3_read:
	mov	%ax, sread
	shl	$9, %cx
	add	%cx, %bx
	jnc	rp_read
	mov	%es, %ax
	add	$0x1000, %ax
	mov	%ax, %es
	xor	%bx, %bx
	jmp	rp_read

read_track:
	push	%ax
	push	%bx
	push	%cx
	push	%dx
	mov	track, %dx
	mov	sread, %cx
	inc	%cx
	mov	%dl, %ch
	mov	head, %dx
	mov	%dl, %dh
	mov	$0, %dl
	and	$0x0100, %dx
	mov	$2, %ah
	int	$0x13
	jc	bad_rt
	pop	%dx
	pop	%cx
	pop	%bx
	pop	%ax
	ret
bad_rt:	mov	$0, %ax
	mov	$0, %dx
	int	$0x13
	pop	%dx
	pop	%cx
	pop	%bx
	pop	%ax
	jmp	read_track
```
### kill_motor函数，软盘电机断电💾
至此，3段Linux0.11的组成——bootsect、setup、system，都已加载完毕。
软盘完成了外部存储OS的任务，将其临时断电，释放资源。

无非也就是向某个端口读写，然后就完成了电机断电。

```
kill_motor:
	push	%dx
	mov	$0x3f2, %dx
	mov	$0, %al
	outsb
	pop	%dx
	ret
```
## (七)bootsect.s 结束的收尾工作
这段代码是bootsect.s（引导扇区代码）的收尾部分，核心作用是定义数据变量、字符串常量、引导扇区标识，以及划分代码 / 数据段边界，是软盘引导扇区必须的 “格式收尾”—— 尤其是**0xAA55标识**，没有它 **BIOS 会认为引导扇区无效。**同时通过边界值的设定(通过`.org 508`指令)，确定bootsect大小为512B,刚好符合规定！
```s
# 定义一个字（2字节）变量sectors，初始值0, 前面代码中会把“每磁道扇区数”写入这个变量
sectors:
	.word 0

msg1:
	.byte 13,10 # .byte定义字节：13=回车(CR)，10=换行(LF)，对应键盘的Enter键
	.ascii "IceCityOS is booting ..."
	.byte 13,10,13,10 # ; 两个回车换行，让提示语下方留空，排版更整洁

	.org 508 # 汇编伪指令：强制后续代码/数据从偏移地址508开始（距离引导扇区起始位置508字节），为了恰好凑齐512KB！

root_dev:
	.word ROOT_DEV #定义字变量root_dev，值为宏ROOT_DEV（根设备号，如0x0208=1.2MB软盘），前面代码中会根据软盘类型自动修改这个值
boot_flag:
	.word 0xAA55 # 引导扇区标识：必须以0xAA55结尾（低字节55，高字节AA）， BIOS会检查引导扇区最后2字节是否为0xAA55，否则拒绝执行引导扇区，为啥一定要是0xAA55？因为A是AT&T，55是U，所以就是为了纪念在AT&T实验室开发出的Unix操作系统
	
	.text
	endtext:    #标记代码段（.text）的结束位置
	.data
	enddata:    #标记数据段（.data）的结束位置
	.bss
	endbss:     #标记未初始化数据段（.bss）的结束位置
```