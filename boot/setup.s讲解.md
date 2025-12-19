# 一、setup.s 的功能
1.将0x00000处的系统元信息保存到0x90000处，包括光标位置、系统参数、引导扇区信息、磁盘参数等。
2.将system模块从0x10000处移动到0x00000处。
3.设置保护模式下的中断描述符表（IDT）和全局描述符表（GDT）。
4.启用A20地址线。
5.通过将cr0寄存器的末尾位PE置为1，进入保护模式。

# 二、按功能逐行讲解setup.s代码

## (一)正式进入_start程序入口前的准备工作
```s
	.code16
	.equ INITSEG, 0x9000	# bootsect数据块的起始位置
	.equ SYSSEG, 0x1000	# system数据块的起始位置
	.equ SETUPSEG, 0x9020	# setup数据块的起始位置

	.global _start, begtext, begdata, begbss, endtext, enddata, endbss
	.text
	begtext:
	.data
	begdata:
	.bss
	begbss:
	.text

	ljmp $SETUPSEG, $_start	# 跳转到setup数据块的起始位置（cs设置为SETUP段地址，ip设置为下面的_start，程序入口）
```