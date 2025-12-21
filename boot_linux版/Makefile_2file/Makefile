all: a.o b.o
	@echo "linking a.o b.o"
	gcc a.o b.o -o ab.exe
	@echo "running ab.exe"
	./ab.exe

a.o: a.c
	@echo "compiling a.c"
	gcc a.c -c -o a.o

b.o: b.c
	@echo "compiling b.c"
	gcc b.c -c -o b.o

clean:
	rm -f *.o *.exe