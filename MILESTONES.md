# SentinelOS Future Milestones

Based on the current state of SentinelOS (which features memory management, scheduling, a VFS abstraction, and an interactive TTY interface running in Ring 0), here is the roadmap for future development.

## 🚀 Milestone 4: User Mode (Ring 3) & System Calls
Right now, everything in the OS runs with ultimate kernel privileges (Ring 0). To provide true security and stability, we need to separate user applications from the kernel.
* **Task State Segment (TSS):** Configure the TSS so the CPU knows where the kernel stack is when an interrupt occurs in user mode.
* **Ring 3 Transition:** Write the assembly code required to drop privileges and jump from Ring 0 to Ring 3 execution.
* **Syscall Interface (Interrupt `0x80`):** Implement a system call handler so user-mode applications can safely request kernel services (e.g., `sys_write` to print to the screen, `sys_yield` for scheduling).

## 📦 Milestone 5: Executable Loading (ELF Parser)
Currently, threads are just C functions compiled directly into the kernel. A real OS loads external programs from a file system and runs them.
* **ELF Parser:** Write a parser to read the standard Linux **Executable and Linkable Format (ELF)** binaries.
* **Process Creation:** When a user types a command, read the ELF file from RamFS, allocate user-mode memory pages for its code and data, map it into a virtual address space, and execute it in Ring 3.

## 💾 Milestone 6: Persistent Storage (ATA/IDE Driver) & FAT32
RamFS is great, but files disappear when the computer turns off. We need to interact with actual hard drives.
* **ATA/IDE PIO Driver:** Write a driver to communicate with standard IDE hard drives to read and write sectors from the disk.
* **Disk Image & GRUB:** Attach a virtual hard disk to QEMU in the `Makefile`.
* **FAT32 / Ext2 Support:** Implement a real file system driver (FAT32 is the easiest start) under the VFS layer, allowing the OS to read actual files and directories off a persistent disk.

## 🧠 Milestone 7: Advanced Memory Management
The current kernel heap (`kmalloc`) is a simple bump-allocator, meaning we can't free memory once it's allocated.
* **Dynamic Allocator:** Implement a proper `kfree` and `kmalloc` using a block-list or slab allocator.
* **User Space Memory:** Implement the `sys_brk` or `sys_mmap` syscalls so user-mode programs can dynamically request memory (like calling `malloc()` in C).
* **Demand Paging:** Only allocate physical memory pages when a process actually tries to use them, optimizing RAM usage.

## 🎨 Milestone 8: Graphics (VESA / Framebuffer)
The 80x25 VGA text mode is nostalgic, but modern OSs use pixel graphics.
* **VESA VBE Mode:** Ask GRUB to set up a high-resolution graphical framebuffer (e.g., 1024x768 with 32-bit color) during boot.
* **Pixel Rendering:** Write drivers to draw individual pixels, rectangles, and load custom fonts.
* **Window Compositor:** Lay the groundwork for a graphical Desktop Environment with movable windows and a mouse cursor (requiring a PS/2 Mouse Driver).
