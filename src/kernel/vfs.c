#include "vfs.h"

vfs_node_t* fs_root = 0;

void vfs_init(void) {
    fs_root = 0;
}

uint32_t vfs_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    if (node->read != 0) return node->read(node, offset, size, buffer);
    return 0;
}

vfs_node_t* vfs_readdir(vfs_node_t* node, uint32_t index) {
    if ((node->flags & FS_DIRECTORY) && node->readdir != 0) {
        return node->readdir(node, index);
    }
    return 0;
}

vfs_node_t* vfs_finddir(vfs_node_t* node, char* name) {
    if ((node->flags & FS_DIRECTORY) && node->finddir != 0) {
        return node->finddir(node, name);
    }
    return 0;
}
