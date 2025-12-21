include ../Makefile.header

LDFLAGS	+= -Ttext 0

all: bootsect setup

# 这是源码，但是为了讲解，所以保留了命令输出@，
# 实际使用时，去不去掉@都可，个人建议不去掉，因为更加直观看到具体编译过程。

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

bootsect: bootsect.s
	$(AS) -o bootsect.o bootsect.s
	$(LD) $(LDFLAGS) -o bootsect bootsect.o
	$(OBJCOPY) -R .pdr -R .comment -R.note -S -O binary bootsect


setup: setup.s
	$(AS) -o setup.o setup.s
	$(LD) $(LDFLAGS) -o setup setup.o
	$(OBJCOPY) -R .pdr -R .comment -R.note -S -O binary setup

head.o: head.s
	$(AS) -o head.o head.s

clean:
	rm -f bootsect bootsect.o setup setup.o head.o