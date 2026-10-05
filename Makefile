CC = gcc
AS = as
LD = ld

CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -O2 -Wall -Wextra
LDFLAGS = -m elf_i386 -T linker.ld

all: kernel.bin

boot.o: boot/boot.s
	$(AS) --32 boot/boot.s -o boot.o

kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) -c kernel/kernel.c -o kernel.o

keyboard.o: kernel/keyboard.c
	$(CC) $(CFLAGS) -c kernel/keyboard.c -o keyboard.o

terminal.o: kernel/terminal.c
	$(CC) $(CFLAGS) -c kernel/terminal.c -o terminal.o

shell.o: kernel/shell.c
	$(CC) $(CFLAGS) -c kernel/shell.c -o shell.o

mouse.o: kernel/mouse.c
	$(CC) $(CFLAGS) -c kernel/mouse.c -o mouse.o

fs.o: kernel/fs.c
	$(CC) $(CFLAGS) -c kernel/fs.c -o fs.o

kernel.bin: boot.o kernel.o keyboard.o terminal.o shell.o mouse.o fs.o
	$(LD) $(LDFLAGS) boot.o kernel.o keyboard.o terminal.o shell.o mouse.o fs.o -o kernel.bin

clean:
	rm -f *.o kernel.bin