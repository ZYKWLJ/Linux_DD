小红书视频：
# 一、作用：
1.将由bios加载到0x7c00的bootsect引导段二进制数据块复制到内存0x9000
2.立马跳转到0x9000处，并且将ds、ss、es都设置为0x9000，sp设置为0xff00，即将堆栈设置为0x9000:0xff00，即0x9ff00
3.将设置段setup二进制数据块加载到内存0x9020
4.将系统段system二进制数据块加载到内存0x10000

# 二、按功能逐行讲解代码

## (一)正式代码执行前的一些赋值

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
	mov	$BOOTSEG, %ax	#将ds段寄存器设置为0x7C0
	mov	%ax, %ds
	mov	$INITSEG, %ax	#将es段寄存器设置为0x900
	mov	%ax, %es
	mov	$256, %cx		#设置移动计数值256字
	sub	%si, %si		#源地址	ds:si = 0x07C0:0x0000
	sub	%di, %di		#目标地址 es:si = 0x9000:0x0000
	rep					#重复执行并递减cx的值
	movsw				#从内存[si]处移动cx个字到[di]处
```

## (三)初始化内存架构，将ds、es、ss赋值为cs的值，即0x9000,并且将sp赋值为0xFF00
```s
	ljmp	$INITSEG, $go	#段间跳转，这里INITSEG指出跳转到的段地址，解释了cs的值为0x9000
go:	mov	%cs, %ax		#将ds，es，ss都设置成移动后代码所在的段处(0x9000)
	mov	%ax, %ds
	mov	%ax, %es
# put stack at 0x9ff00.
	mov	%ax, %ss
	mov	$0xFF00, %sp		# arbitrary value >>512
```