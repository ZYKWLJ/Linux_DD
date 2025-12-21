CC	=gcc # -march=i386 $(RAMDISK)
AS =as
CFLAGS	=-Wall -m32 -O2 -fomit-frame-pointer -fno-builtin
LD	=ld
LDFLAGS	  = -m  i386pe  -M 
# 显式规则：
test1.o: test1.c
	$(CC) $(CFLAGS) \
	-c -o test1.o test1.c

test2.o: test2.c
	$(CC) $(CFLAGS) \
	-c -o test2.o test2.c

# 我们可以看到，显示规则不解耦，很麻烦，需要专门定制。即使他们的逻辑相似，也要再重新写一遍。很麻烦。有没有什么办法解决？有！隐式规则！


# 隐式规则：
# 也称为后缀规则 - .c文件转.s文件（源码编译为汇编代码）
# 可以匹配任意符合该规则的命令。make name.s,则会找到生成.s的规则。并完成名字替换。
#  后缀规则 - .c文件转.o文件（C源码编译为目标文件）
.c.o:
	$(CC) $(CFLAGS) \
	-c -o $*.o $< 
# 解释：-c: 只编译不链接，生成.o目标文件

# 同理，还有如下的：
# 定义 “把任意 .c 后缀的 C 语言源码文件，编译为同名 .s 后缀的汇编代码文件” 的自动化编译命令。
.c.s: 
	$(CC) $(CFLAGS) \
	-S -o $*.s $<

# 后缀规则 - .s文件转.o文件（汇编源码编译为目标文件）
.s.o:
	$(AS) $(CFLAGS) \
	-c  -o $*.o $<

test1.exe: test1.o
	$(LD) $(LDFLAGS) \
	-o test1.exe $< >System.map

clean:
	rm -rf *.o *.exe *.s

.phony:clean