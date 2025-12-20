all: a.o
	@echo "linking a.o"
	gcc a.o -o a.exe
	@echo "running a.exe"
	./a.exe

a.o: a.c
	@echo "compiling a.c"
	gcc a.c -c -o a.o

clean:
	rm -f *.o *.exe