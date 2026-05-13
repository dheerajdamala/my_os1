# SentinelOS Documentation

SentinelOS is a custom-built, security-first microkernel operating system written from scratch in C and x86 Assembly. It runs in 32-bit Protected Mode and features preemptive multitasking, asynchronous IPC, identity paging, and an aesthetic GenZ-inspired TTY interface.

---

## 🚀 Development Milestones

### Pre-Milestone: The MVP Foundation
**What we did**: Established the bare-metal foundation of the OS.
**How we did it**:
- **Bootloader**: Wrote `boot.asm` to comply with the Multiboot specification, initialize a stack, and transition execution to the C kernel.
- **Interrupts (GDT & IDT)**: Configured the Global Descriptor Table and Interrupt Descriptor Table to handle CPU exceptions and hardware IRQs.
- **Preemptive Scheduler**: Implemented a round-robin thread scheduler running at Ring 0, driven by the Programmable Interval Timer (PIT) at 100Hz.
- **Context Switching**: Wrote custom Assembly (`switch.asm`) to safely save and restore registers across thread preemptions and yields.
- **Memory Management**: Built a bitmap-based Physical Memory Manager (PMM) and a basic bump-allocator Kernel Heap (`kmalloc`).

### Milestone 1: Interactive Shell & TTY Aesthetics
**What we did**: Transformed the OS from headless serial logs to an interactive, visually pleasing VGA text mode environment.
**How we did it**:
- **VGA Driver**: Created `vga.c` and `tty.c` to take control of the 80x25 text mode buffer. Added support for text colors (e.g., cyan on black).
- **Dashboard Telemetry**: Designed a bottom-bar HUD (`dashboard.c`) showing real-time stats like Thread Count, Memory Usage, Uptime, and Context Switches.
- **Keyboard Driver**: Implemented `keyboard.c` utilizing a ring buffer. Initially interrupt-based, it was later refactored to poll the PS/2 controller on every timer tick to bypass QEMU IRQ routing bugs and ensure absolute stability.
- **Shell Thread**: Created an interactive command-line interface (`shell.c`) with an input prompt (`sentinel>`) allowing users to type and execute built-in commands (e.g., `help`, `uptime`, `mem`).

### Milestone 2: Memory Protection & Paging
**What we did**: Implemented hardware-level memory protection to prevent kernel crashes due to memory expansion.
**How we did it**:
- **x86 Paging**: Wrote `paging.c` to enable 32-bit paging in the CPU's CR0/CR3 registers.
- **Identity Mapping**: Mapped the first 16 MB of RAM (identity mapped: physical address = virtual address) using a Page Directory and 4 Page Tables.
- **Page Fault Handler**: Registered an ISR (Interrupt Service Routine) for Interrupt 14 to catch memory access violations, printing the faulting address to the screen.

### Milestone 3: Virtual File System & "GenZ" Tools
**What we did**: Introduced a file system structure and aesthetic CLI tools to give the OS a modern, fun feel.
**How we did it**:
- **VFS & RamFS**: Implemented `vfs.c` to standardize file interactions and `ramfs.c` to create an in-memory file system. Populated the root directory with dummy files (`readme.txt`, `config.sys`).
- **File Commands**: Added `ls` and `cat` to the shell to explore the RamFS.
- **SentinelFetch (`fetch`)**: Created a beautiful, colorful system information tool (inspired by `neofetch`) using extended VGA color attributes.
- **Digital Rain (`matrix`)**: Wrote a custom TTY-based animation mimicking the famous "Matrix code" effect, driven smoothly by the PIT timer ticks.

---

## 🛠️ Prerequisites & Dependencies

To compile and run SentinelOS on a Debian/Ubuntu-based system, install the following packages:

```bash
sudo apt-get update
sudo apt-get install -y gcc-multilib nasm xorriso mtools grub-common grub-pc-bin qemu-system-x86 make
```

- **`gcc-multilib`**: Required to cross-compile 32-bit binaries (`-m32`) on a 64-bit host.
- **`nasm`**: The Netwide Assembler, used for compiling `.asm` files.
- **`xorriso`, `mtools`, `grub-common`, `grub-pc-bin`**: Used by `grub-mkrescue` to generate the bootable ISO.
- **`qemu-system-x86`**: The emulator used to run the OS.

---

## ⚙️ Building and Running

### 1. Build the OS ISO
To compile all C/Assembly source files and package them into a GRUB-bootable ISO (`sentinel.iso`), run:
```bash
make
```

### 2. Run in QEMU (Interactive GUI)
To launch the OS with the graphical window and full keyboard support, run:
```bash
make run
```
*Note: This command explicitly uses `-machine pc` and `-display gtk,grab-on-hover=on` to guarantee reliable PS/2 keyboard emulation in QEMU. Just hover your mouse over the QEMU window to start typing.*

### 3. Run in QEMU (Headless)
If you only want to capture the serial port logs without a UI (useful for CI/CD), run:
```bash
make run-headless
```
This runs QEMU in the background, waits a few seconds, terminates it, and prints the generated `serial.log`.

### 4. Clean the Build
To remove all object files and the generated ISO:
```bash
make clean
```

---

## 📂 Directory Structure

- `Makefile`: Build scripts and QEMU run targets.
- `src/boot/`: Multiboot entry point (`boot.asm`).
- `src/kernel/`: Core OS components (GDT, IDT, Paging, Scheduler, IPC, Timer, VFS, Shell, Aesthetic Tools).
- `src/drivers/`: Hardware drivers (VGA, TTY, Keyboard, Serial).
- `src/include/`: C header files for all subsystems.
- `src/linker.ld`: The linker script orchestrating the memory layout.
- `iso/boot/grub/grub.cfg`: GRUB boot menu configuration.