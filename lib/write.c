/*
 *  linux/lib/write.c
 *
 *  (C) 1991  Linus Torvalds
 *
 *  这是Linux内核库中的write系统调用封装函数实现文件
 *  作用是为用户空间程序提供write系统调用的接口
 */

// 定义__LIBRARY__宏，用于启用unistd.h中的系统调用宏定义
#define __LIBRARY__
#include <unistd.h>

/*
 *  使用_syscall3宏定义write系统调用的封装函数
 *  宏参数说明：
 *  - 返回值类型：int（返回写入的字节数，出错时返回-1）
 *  - 函数名：write（系统调用名称）
 *  - 参数1类型：int，参数名：fd（文件描述符，指定要写入的文件）
 *  - 参数2类型：const char *，参数名：buf（指向要写入数据的缓冲区指针）
 *  - 参数3类型：off_t，参数名：count（要写入的字节数）
 *
 *  该宏会展开为一个函数，通过软中断指令(int 0x80)触发系统调用
 *  将系统调用号(__NR_write)存入eax寄存器，参数依次存入ebx、ecx、edx寄存器
 */
_syscall3(int, write, int, fd, const char *, buf, off_t, count)