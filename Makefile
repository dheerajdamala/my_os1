# Makefile for SentinelOS

# Tools
CC = gcc
AS = nasm
LD = ld

# Flags
CFLAGS = -m32 -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Isrc/include -fno-pie -fno-stack-protector
LDFLAGS = -m elf_i386 -T src/linker.ld -nostdlib

# Source files
ASM_SRCS = $(wildcard src/boot/*.asm) $(wildcard src/kernel/*.asm)
C_SRCS = $(wildcard src/kernel/*.c) $(wildcard src/drivers/*.c)

# Object files
ASM_OBJS = $(ASM_SRCS:.asm=.o)
C_OBJS = $(C_SRCS:.c=.o)
OBJS = $(ASM_OBJS) $(C_OBJS)

# Target
TARGET = iso/boot/sentinel.bin
ISO_TARGET = sentinel.iso

.PHONY: all clean run

all: $(ISO_TARGET)

# Compile assembly
%.o: %.asm
	$(AS) -f elf32 $< -o $@

# Compile C
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link kernel
$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(TARGET)

# Build ISO
$(ISO_TARGET): $(TARGET)
	grub-mkrescue -o $(ISO_TARGET) iso

# Run with display (interactive — keyboard works via PS/2)
run: $(ISO_TARGET) disk.img
	qemu-system-i386 -machine pc -cdrom $(ISO_TARGET) -hda disk.img -serial file:serial.log -display gtk,grab-on-hover=on

# Headless run (for CI / serial log only)
run-headless: $(ISO_TARGET) disk.img
	qemu-system-i386 -machine pc -cdrom $(ISO_TARGET) -hda disk.img -serial file:serial.log -display none &
	sleep 2
	killall qemu-system-i386 || true

disk.img: test_program.elf
	dd if=/dev/zero of=disk.img bs=1M count=36
	mkfs.vfat -F 32 disk.img
	echo "Hello from the persistent FAT32 storage!" > test.txt
	mcopy -i disk.img test.txt ::test.txt
	mmd -i disk.img ::docs
	echo "Inside docs directory on disk." > doc1.txt
	mcopy -i disk.img doc1.txt ::docs/doc1.txt
	mcopy -i disk.img test_program.elf ::test.elf
	rm -f test.txt doc1.txt

test_program.elf: test_program.c user_linker.ld
	$(CC) -m32 -static -ffreestanding -O2 -nostdlib -T user_linker.ld -o test_program.elf test_program.c

clean:
	rm -f $(OBJS) $(TARGET) $(ISO_TARGET) disk.img test_program.elf
