# include ../Makefile.header

# LDFLAGS	+= -Ttext 0

# all: bootsect setup

# bootsect: bootsect.s
# 	@$(AS) -o bootsect.o bootsect.s
# 	@$(LD) $(LDFLAGS) -o bootsect bootsect.o
# 	@$(OBJCOPY) -R .pdr -R .comment -R.note -S -O binary bootsect


# setup: setup.s
# 	@$(AS) -o setup.o setup.s
# 	@$(LD) $(LDFLAGS) -o setup setup.o
# 	@$(OBJCOPY) -R .pdr -R .comment -R.note -S -O binary setup

# head.o: head.s
# 	@$(AS) -o head.o head.s

# clean:
# 	@rm -f bootsect bootsect.o setup setup.o head.o

# test: test.c
# 	@echo "compiling test.c"
# 	gcc test.c -o test
# 	@echo "running test.c"
# 	./test

# # 1. 定义默认目标：先编译，再运行
# all: test run

# # 2. 编译规则：仅test.c更新时才重新编译
# test: test.c
# 	@echo "compiling test.c"
# 	gcc test.c -o test.exe

# # 3. 运行规则：依赖test文件，仅test更新时才运行
# run: test.exe
# 	@echo "running test.c"
# 	./test

# # 声明伪目标（避免和同名文件冲突）
# .PHONY: all run


# 1. 定义默认目标：先编译，再运行
all: test run

# 2. 编译规则：仅test.c更新时才重新编译
test: test.c
	@echo "compiling test.c"
	gcc test.c -o test.exe

# 3. 运行规则：依赖test文件，仅test更新时才运行
run: test.exe
	@echo "running test.c"
	./test.exe

# 4.清理目标
clean:
	@echo "cleaning all exe && obj files"
	-rm -f *.exe *.o

# 声明伪目标（避免和同名文件冲突）
.PHONY: all run