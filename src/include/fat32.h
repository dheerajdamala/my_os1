#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include "vfs.h"

/* FAT32 BPB and Boot Sector Structure */
typedef struct {
    uint8_t bootjmp[3];
    uint8_t oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t table_count;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t media_type;
    uint16_t table_size_16;
    uint16_t sectors_per_track;
    uint16_t head_side_count;
    uint32_t hidden_sector_count;
    uint32_t total_sectors_32;
    
    /* FAT32 extended fields */
    uint32_t table_size_32;
    uint16_t extended_flags;
    uint16_t fat_version;
    uint32_t root_cluster;
    uint16_t fat_info;
    uint16_t backup_BS_sector;
    uint8_t reserved_0[12];
    uint8_t drive_number;
    uint8_t reserved_1;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t fat_type_label[8];
} __attribute__((packed)) fat32_bpb_t;

/* FAT32 32-Byte Directory Entry Structure */
typedef struct {
    uint8_t name[11];
    uint8_t attr;
    uint8_t nt_res;
    uint8_t crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t lst_acc_date;
    uint16_t first_cluster_hi;
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t first_cluster_lo;
    uint32_t file_size;
} __attribute__((packed)) fat32_dir_entry_t;

/* Internal VFS structure representing a FAT32 node metadata */
typedef struct {
    uint32_t first_cluster;
    uint32_t size;
    uint32_t is_dir;
} fat32_vfs_info_t;

void fat32_init(void);
vfs_node_t* fat32_get_root_node(void);

#endif // FAT32_H
