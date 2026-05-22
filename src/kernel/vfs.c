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
    if (!node || !name || name[0] == '\0') return 0;
    
    // Skip leading slashes
    while (name[0] == '/') name++;
    if (name[0] == '\0') return node;
    
    // Find length of first component
    int len = 0;
    while (name[len] && name[len] != '/') {
        len++;
    }
    
    // Copy first component to a local buffer
    char component[128];
    if (len >= 128) len = 127;
    for (int i = 0; i < len; i++) {
        component[i] = name[i];
    }
    component[len] = '\0';
    
    // Call the underlying finddir for this component
    vfs_node_t* child = 0;
    if ((node->flags & FS_DIRECTORY) && node->finddir != 0) {
        child = node->finddir(node, component);
    }
    
    if (!child) return 0;
    
    // If there is more path, recurse
    if (name[len] == '/') {
        return vfs_finddir(child, name + len + 1);
    }
    
    return child;
}
