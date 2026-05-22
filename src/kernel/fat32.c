#include "fat32.h"
#include "ata.h"
#include "serial.h"
#include "memory.h"

#define MAX_FAT32_NODES 64

static fat32_bpb_t bpb;
static uint32_t data_start_sector = 0;
static uint32_t fat_start_sector = 0;
static uint32_t sectors_per_cluster = 0;

static vfs_node_t fat32_nodes[MAX_FAT32_NODES];
static fat32_vfs_info_t fat32_node_infos[MAX_FAT32_NODES];
static vfs_node_t fat32_root_node;
static fat32_vfs_info_t fat32_root_info;

/* Simple string helpers */
static int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

static void fat_name_to_string(const uint8_t* fat_name, char* out_name) {
    int name_len = 0;
    
    // Process base name (first 8 chars)
    for (int i = 0; i < 8; i++) {
        if (fat_name[i] != ' ') {
            char c = fat_name[i];
            if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
            out_name[name_len++] = c;
        }
    }
    
    // Process extension (remaining 3 chars)
    int ext_len = 0;
    char ext[4];
    for (int i = 8; i < 11; i++) {
        if (fat_name[i] != ' ') {
            char c = fat_name[i];
            if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
            ext[ext_len++] = c;
        }
    }
    
    if (ext_len > 0) {
        out_name[name_len++] = '.';
        for (int i = 0; i < ext_len; i++) {
            out_name[name_len++] = ext[i];
        }
    }
    out_name[name_len] = '\0';
}

static vfs_node_t* get_next_free_node(void) {
    static int next_node = 0;
    vfs_node_t* n = &fat32_nodes[next_node];
    fat32_vfs_info_t* info = &fat32_node_infos[next_node];
    next_node = (next_node + 1) % MAX_FAT32_NODES;
    
    // Setup connection
    n->ptr = info;
    return n;
}

uint32_t fat32_get_next_cluster(uint32_t current_cluster) {
    uint8_t sector_buf[512];
    uint32_t fat_sector = fat_start_sector + (current_cluster * 4) / 512;
    uint32_t fat_offset = (current_cluster * 4) % 512;
    
    ata_read_sector(fat_sector, sector_buf);
    uint32_t next_cluster = *(uint32_t*)&sector_buf[fat_offset];
    return next_cluster & 0x0FFFFFFF; // Mask top 4 reserved bits
}

int fat32_readdir(uint32_t dir_cluster, uint32_t index, fat32_dir_entry_t* out_entry) {
    uint32_t cluster = dir_cluster;
    uint32_t current_idx = 0;
    uint8_t sector_buf[512];
    
    while (cluster >= 2 && cluster < 0x0FFFFFF8) {
        uint32_t first_sector = data_start_sector + (cluster - 2) * sectors_per_cluster;
        
        for (uint32_t s = 0; s < sectors_per_cluster; s++) {
            ata_read_sector(first_sector + s, sector_buf);
            
            for (int e = 0; e < 16; e++) {
                fat32_dir_entry_t* entry = (fat32_dir_entry_t*)&sector_buf[e * 32];
                
                // End of directory indicator
                if (entry->name[0] == 0x00) return 0;
                
                // Deleted/free directory entry
                if (entry->name[0] == 0xE5) continue;
                
                // Long File Name entry
                if (entry->attr == 0x0F) continue;
                
                if (current_idx == index) {
                    *out_entry = *entry;
                    return 1;
                }
                current_idx++;
            }
        }
        
        cluster = fat32_get_next_cluster(cluster);
    }
    return 0;
}

vfs_node_t* fat32_vfs_readdir(vfs_node_t* node, uint32_t index);
vfs_node_t* fat32_vfs_finddir(vfs_node_t* node, char* name);
uint32_t fat32_vfs_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);

vfs_node_t* fat32_vfs_readdir(vfs_node_t* node, uint32_t index) {
    fat32_vfs_info_t* parent_info = (fat32_vfs_info_t*)node->ptr;
    fat32_dir_entry_t entry;
    
    if (fat32_readdir(parent_info->first_cluster, index, &entry)) {
        vfs_node_t* child = get_next_free_node();
        fat32_vfs_info_t* child_info = (fat32_vfs_info_t*)child->ptr;
        
        fat_name_to_string(entry.name, child->name);
        child->length = entry.file_size;
        
        child_info->first_cluster = ((uint32_t)entry.first_cluster_hi << 16) | entry.first_cluster_lo;
        child_info->size = entry.file_size;
        
        if (entry.attr & 0x10) { // Directory
            child->flags = FS_DIRECTORY;
            child->read = 0;
            child->readdir = fat32_vfs_readdir;
            child->finddir = fat32_vfs_finddir;
            child_info->is_dir = 1;
        } else { // File
            child->flags = FS_FILE;
            child->read = fat32_vfs_read;
            child->readdir = 0;
            child->finddir = 0;
            child_info->is_dir = 0;
        }
        return child;
    }
    return 0;
}

vfs_node_t* fat32_vfs_finddir(vfs_node_t* node, char* name) {
    fat32_vfs_info_t* parent_info = (fat32_vfs_info_t*)node->ptr;
    fat32_dir_entry_t entry;
    
    int index = 0;
    while (fat32_readdir(parent_info->first_cluster, index++, &entry)) {
        char entry_name[128];
        fat_name_to_string(entry.name, entry_name);
        
        if (strcmp(entry_name, name) == 0) {
            vfs_node_t* child = get_next_free_node();
            fat32_vfs_info_t* child_info = (fat32_vfs_info_t*)child->ptr;
            
            int i = 0;
            while (entry_name[i]) {
                child->name[i] = entry_name[i];
                i++;
            }
            child->name[i] = '\0';
            child->length = entry.file_size;
            
            child_info->first_cluster = ((uint32_t)entry.first_cluster_hi << 16) | entry.first_cluster_lo;
            child_info->size = entry.file_size;
            
            if (entry.attr & 0x10) {
                child->flags = FS_DIRECTORY;
                child->read = 0;
                child->readdir = fat32_vfs_readdir;
                child->finddir = fat32_vfs_finddir;
                child_info->is_dir = 1;
            } else {
                child->flags = FS_FILE;
                child->read = fat32_vfs_read;
                child->readdir = 0;
                child->finddir = 0;
                child_info->is_dir = 0;
            }
            return child;
        }
    }
    return 0;
}

uint32_t fat32_vfs_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    fat32_vfs_info_t* info = (fat32_vfs_info_t*)node->ptr;
    if (offset >= info->size) return 0;
    if (offset + size > info->size) size = info->size - offset;
    
    uint32_t bytes_read = 0;
    uint32_t current_cluster = info->first_cluster;
    uint32_t cluster_size = sectors_per_cluster * 512;
    
    // Skip to target cluster
    uint32_t skip_clusters = offset / cluster_size;
    for (uint32_t i = 0; i < skip_clusters; i++) {
        current_cluster = fat32_get_next_cluster(current_cluster);
        if (current_cluster < 2 || current_cluster >= 0x0FFFFFF8) {
            return 0;
        }
    }
    
    uint32_t cluster_offset = offset % cluster_size;
    uint8_t* cluster_buf = (uint8_t*)kmalloc(cluster_size);
    if (!cluster_buf) {
        serial_printf("[FAT32] Out of memory allocating cluster buffer!\n");
        return 0;
    }
    
    while (bytes_read < size && current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
        uint32_t first_sector = data_start_sector + (current_cluster - 2) * sectors_per_cluster;
        
        // Read sectors belonging to current cluster
        for (uint32_t s = 0; s < sectors_per_cluster; s++) {
            if (s * 512 < cluster_size) {
                ata_read_sector(first_sector + s, cluster_buf + s * 512);
            }
        }
        
        uint32_t copy_len = cluster_size - cluster_offset;
        if (copy_len > (size - bytes_read)) copy_len = size - bytes_read;
        
        for (uint32_t i = 0; i < copy_len; i++) {
            buffer[bytes_read + i] = cluster_buf[cluster_offset + i];
        }
        
        bytes_read += copy_len;
        cluster_offset = 0;
        current_cluster = fat32_get_next_cluster(current_cluster);
    }
    
    kfree(cluster_buf);
    return bytes_read;
}

void fat32_init(void) {
    uint8_t sector_buf[512];
    
    serial_printf("[FAT32] Initializing FAT32 reader...\n");
    ata_read_sector(0, sector_buf);
    
    // Copy sector 0 into BPB
    uint8_t* bpb_ptr = (uint8_t*)&bpb;
    for (int i = 0; i < (int)sizeof(fat32_bpb_t); i++) {
        bpb_ptr[i] = sector_buf[i];
    }
    
    // Check standard FAT format signatures
    if (bpb.boot_signature != 0x29) {
        serial_printf("[FAT32] Error: Invalid boot signature (0x%x). FAT32 mount failed.\n", bpb.boot_signature);
        return;
    }
    
    sectors_per_cluster = bpb.sectors_per_cluster;
    fat_start_sector = bpb.reserved_sector_count;
    data_start_sector = bpb.reserved_sector_count + (bpb.table_count * bpb.table_size_32);
    
    // Initialize root directory node
    int name_idx = 0;
    const char* root_name = "disk";
    while (root_name[name_idx]) {
        fat32_root_node.name[name_idx] = root_name[name_idx];
        name_idx++;
    }
    fat32_root_node.name[name_idx] = '\0';
    
    fat32_root_node.flags = FS_DIRECTORY;
    fat32_root_node.length = 0;
    fat32_root_node.read = 0;
    fat32_root_node.readdir = fat32_vfs_readdir;
    fat32_root_node.finddir = fat32_vfs_finddir;
    
    fat32_root_info.first_cluster = bpb.root_cluster;
    fat32_root_info.size = 0;
    fat32_root_info.is_dir = 1;
    fat32_root_node.ptr = &fat32_root_info;
    
    // Initialize static node pool link ptrs
    for (int i = 0; i < MAX_FAT32_NODES; i++) {
        fat32_nodes[i].ptr = &fat32_node_infos[i];
    }
    
    serial_printf("[FAT32] Mounted successfully. Sectors/cluster: %d, Root Cluster: %d, Data Sector: %d\n",
                  sectors_per_cluster, bpb.root_cluster, data_start_sector);
}

vfs_node_t* fat32_get_root_node(void) {
    if (data_start_sector == 0) return 0; // Not initialized
    return &fat32_root_node;
}
