/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+                                                               +
+  本文链接：https://mp.weixin.qq.com/s/MlItVZeXmqZAufZPrjWbGQ  
+  文件功能：  io.h定义了一系列宏，这些宏是 x86 架构下直接操作硬件 I/O 端口的底层工具
+                                                               +
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
#define outb(value,port) \
__asm__ ("outb %%al,%%dx"::"a" (value),"d" (port))


#define inb(port) ({ \
unsigned char _v; \
__asm__ volatile ("inb %%dx,%%al":"=a" (_v):"d" (port)); \
_v; \
})

#define outb_p(value,port) \
__asm__ ("outb %%al,%%dx\n" \
		"\tjmp 1f\n" \
		"1:\tjmp 1f\n" \
		"1:"::"a" (value),"d" (port))

#define inb_p(port) ({ \
unsigned char _v; \
__asm__ volatile ("inb %%dx,%%al\n" \
	"\tjmp 1f\n" \
	"1:\tjmp 1f\n" \
	"1:":"=a" (_v):"d" (port)); \
_v; \
})
