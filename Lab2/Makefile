CC=	clang
CFLAGS=	-Weverything -g
LDFLAGS=-no-pie

all: syscalls
clean:
	rm -f syscalls *.o

syscalls: main.o stuff.o
	$(CC) main.o stuff.o -o syscalls