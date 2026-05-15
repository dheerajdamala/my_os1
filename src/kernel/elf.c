#include "elf.h"
#include "vfs.h"
#include "serial.h"
#include "memory.h"

uint32_t elf_load_file(const char* path) {
    vfs_node_t* node = vfs_finddir(fs_root, (char*)path);
    if (!node) {
        serial_printf("ELF: File not found: %s\n", path);
        return 0;
    }

    elf_header_t header;
    vfs_read(node, 0, sizeof(elf_header_t), (uint8_t*)&header);

    if (*(uint32_t*)header.ident != 0x464C457F) {
        serial_printf("ELF: Invalid magic\n");
        return 0;
    }

    serial_printf("ELF: Loading %s, entry 0x%x, phnum %d\n", path, header.entry, header.phnum);

    for (int i = 0; i < header.phnum; i++) {
        elf_program_header_t ph;
        vfs_read(node, header.phoff + i * header.phentsize, sizeof(elf_program_header_t), (uint8_t*)&ph);

        if (ph.type == 1) { // PT_LOAD
            serial_printf("ELF: Loading segment at 0x%x (filesz: %d, memsz: %d)\n", ph.vaddr, ph.filesz, ph.memsz);
            vfs_read(node, ph.offset, ph.filesz, (uint8_t*)ph.vaddr);
            
            if (ph.memsz > ph.filesz) {
                uint8_t* bss = (uint8_t*)(ph.vaddr + ph.filesz);
                for (uint32_t j = 0; j < (ph.memsz - ph.filesz); j++) bss[j] = 0;
            }
        }
    }

    return header.entry;
}
