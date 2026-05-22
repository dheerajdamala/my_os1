#ifndef RAMFS_H
#define RAMFS_H

#include "vfs.h"

void ramfs_init(void);
void ramfs_create_mountpoint(const char* name, vfs_node_t* mount_root);

#endif
