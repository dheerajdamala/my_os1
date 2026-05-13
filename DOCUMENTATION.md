# SentinelOS - MVP Implementation

SentinelOS is a security-first microkernel operating system built from scratch. This repository contains the Minimum Viable Product (MVP) that runs in 32-bit Protected Mode on x86_64 architecture.

## Features Implemented

The MVP currently includes the following foundational OS components:
- **Bootloader**: A Multiboot-compliant entry point (`boot.asm`) that initializes the stack and transitions to the C kernel.
- **Serial Driver**: A headless output driver that logs data over the COM1 serial port, allowing testing via QEMU without a graphical display.
- **GDT & IDT**: Global and Interrupt Descriptor Tables to manage hardware interrupts and exceptions.
- **Timer (PIT)**: A Programmable Interval Timer configured at 100Hz to drive the preemptive scheduler.
- **Memory Management**: A bitmap-based Physical Memory Manager (PMM) and a basic bump-allocator Kernel Heap (`kmalloc`).
- **Preemptive Scheduler**: A round-robin thread scheduler running in Ring 0, featuring custom Assembly context switching.
- **Inter-Process Communication (IPC)**: A mailbox-based asynchronous message-passing system enabling threads to communicate.
- **Kernel Telemetry Framework (KTF)**: A native observability subsystem that logs structured JSON telemetry (thread creation, scheduler switches, IPC events) to the serial port.

## Prerequisites & Dependencies

To build and run SentinelOS on a Debian/Ubuntu-based system, you need the following packages:

```bash
sudo apt-get update
sudo apt-get install -y gcc-multilib nasm xorriso mtools grub-common grub-pc-bin qemu-system-x86 make
```

- **`gcc-multilib`**: Required to cross-compile 32-bit binaries (`-m32`) on a 64-bit host.
- **`nasm`**: The Netwide Assembler, used for compiling the `.asm` files.
- **`xorriso`, `mtools`, `grub-common`, `grub-pc-bin`**: Used by `grub-mkrescue` to generate the bootable ISO.
- **`qemu-system-x86`**: The emulator used to run the OS.

## Building and Running

1. **Build the Kernel ISO:**
   Run the following command to compile the C and Assembly files and package them into `sentinel.iso` using GRUB:
   ```bash
   make
   ```

2. **Run in QEMU (Headless):**
   The `Makefile` is configured to boot the ISO headlessly and route all serial output to a file. Run:
   ```bash
   make run
   ```
   This command starts QEMU, waits a couple of seconds for the OS to boot and log its telemetry, then kills the QEMU process and prints the output of `serial.log` to your terminal.

3. **Clean the build directory:**
   ```bash
   make clean
   ```

## Directory Structure

- `src/boot/`: Contains the Multiboot entry point (`boot.asm`).
- `src/kernel/`: Contains the core C and Assembly files for the kernel, including GDT, IDT, Timer, Memory, Scheduler, IPC, and KTF.
- `src/drivers/`: Contains the Serial Port driver (`serial.c`).
- `src/include/`: Contains the C header definitions.
- `src/linker.ld`: The linker script defining the kernel memory layout.
- `iso/boot/grub/grub.cfg`: The GRUB configuration file.