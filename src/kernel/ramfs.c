#include "ramfs.h"
#include "test_program_elf.h"

#define MAX_RAMFS_FILES 16

typedef struct {
    vfs_node_t node;
    uint8_t* data;
} ramfs_file_t;

static ramfs_file_t files[MAX_RAMFS_FILES];
static int num_files = 0;
static vfs_node_t ramfs_root;

static uint32_t ramfs_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    ramfs_file_t* file = (ramfs_file_t*)node->ptr;
    if (offset >= node->length) return 0;
    if (offset + size > node->length) size = node->length - offset;
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = file->data[offset + i];
    }
    return size;
}

static vfs_node_t* ramfs_readdir(vfs_node_t* node, uint32_t index) {
    (void)node;
    if (index < (uint32_t)num_files) {
        return &files[index].node;
    }
    return 0;
}

static int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

static void strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

static vfs_node_t* ramfs_finddir(vfs_node_t* node, char* name) {
    (void)node;
    for (int i = 0; i < num_files; i++) {
        if (strcmp(files[i].node.name, name) == 0) {
            return &files[i].node;
        }
    }
    return 0;
}

void ramfs_create_binary_file(const char* name, uint8_t* data, uint32_t size) {
    if (num_files >= MAX_RAMFS_FILES) return;
    
    ramfs_file_t* f = &files[num_files++];
    strcpy(f->node.name, name);
    f->node.flags = FS_FILE;
    f->node.length = size;
    
    f->node.read = ramfs_read;
    f->node.finddir = 0;
    f->node.readdir = 0;
    f->data = data;
    f->node.ptr = f;
}

static void create_file(const char* name, const char* content) {
    if (num_files >= MAX_RAMFS_FILES) return;
    
    ramfs_file_t* f = &files[num_files++];
    strcpy(f->node.name, name);
    f->node.flags = FS_FILE;
    
    uint32_t len = 0;
    while (content[len]) len++;
    f->node.length = len;
    
    f->node.read = ramfs_read;
    f->node.finddir = 0;
    f->node.readdir = 0;
    f->data = (uint8_t*)content;
    f->node.ptr = f;
}

void ramfs_init(void) {
    strcpy(ramfs_root.name, "root");
    ramfs_root.flags = FS_DIRECTORY;
    ramfs_root.read = 0;
    ramfs_root.readdir = ramfs_readdir;
    ramfs_root.finddir = ramfs_finddir;
    
    fs_root = &ramfs_root;
    
    create_file("readme.txt", "Welcome to SentinelOS!\nThis is a minimal in-memory file system (RamFS).\n");
    create_file("secrets.txt", "TOP SECRET: The cake is a lie.\n");
    create_file("config.sys", "OS=SentinelOS\nVERSION=0.1.0\nARCH=x86-32\nAESTHETICS=MAXIMUM\n");
    ramfs_create_binary_file("test.elf", test_program_elf, test_program_elf_len);
}
