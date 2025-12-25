CC=gcc
LD=ld
OBJECTS=extern.o
TARGET=extern.exe
all: $(TARGET)

# 现代模式规则（替代老式的.c.o:，更清晰）：编译.c生成.o
%.o: %.c
	$(CC) \
	-c -o $@ $<  
	
	# $@=目标.o文件（extern.o），$<=依赖.c文件（extern.c）

$(TARGET): $(OBJECTS)
	$(CC) \
	-o $@ $<