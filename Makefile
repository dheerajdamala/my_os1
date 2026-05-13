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

# Run in QEMU
run: $(ISO_TARGET)
	qemu-system-i386 -cdrom $(ISO_TARGET) -nographic -serial file:serial.log &
	sleep 2
	killall qemu-system-i386 || true
	cat serial.log

clean:
	rm -f $(OBJS) $(TARGET) $(ISO_TARGET)
