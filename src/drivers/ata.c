#include "ata.h"
#include "io.h"
#include "serial.h"

/* ATA IO Ports (Primary Bus, Master Drive) */
#define ATA_PORT_DATA         0x1F0
#define ATA_PORT_ERROR        0x1F1
#define ATA_PORT_SEC_COUNT    0x1F2
#define ATA_PORT_LBA_LOW      0x1F3
#define ATA_PORT_LBA_MID      0x1F4
#define ATA_PORT_LBA_HIGH     0x1F5
#define ATA_PORT_DRIVE        0x1F6
#define ATA_PORT_STATUS       0x1F7
#define ATA_PORT_COMMAND      0x1F7

/* ATA Commands */
#define ATA_CMD_READ          0x20
#define ATA_CMD_WRITE         0x30
#define ATA_CMD_CACHE_FLUSH   0xE7

/* Status bits */
#define ATA_SR_BSY            0x80
#define ATA_SR_DRDY           0x40
#define ATA_SR_DF             0x20
#define ATA_SR_DSC            0x10
#define ATA_SR_DRQ            0x08
#define ATA_SR_CORR           0x04
#define ATA_SR_IDX            0x02
#define ATA_SR_ERR            0x01

static void ata_wait_bsy(void) {
    while (inb(ATA_PORT_STATUS) & ATA_SR_BSY);
}

static void ata_wait_drq(void) {
    while (!(inb(ATA_PORT_STATUS) & ATA_SR_DRQ));
}

void ata_init(void) {
    serial_printf("[ATA] Initializing Primary Master ATA Drive...\n");
    // Send standard drive select
    outb(ATA_PORT_DRIVE, 0xA0);
    ata_wait_bsy();
    serial_printf("[ATA] Primary Master ready.\n");
}

void ata_read_sector(uint32_t lba, uint8_t* buf) {
    ata_wait_bsy();

    // Select drive (LBA mode, master drive, upper bits of LBA)
    outb(ATA_PORT_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    // Number of sectors to read
    outb(ATA_PORT_SEC_COUNT, 1);
    // Send LBA bits
    outb(ATA_PORT_LBA_LOW, (uint8_t)lba);
    outb(ATA_PORT_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PORT_LBA_HIGH, (uint8_t)(lba >> 16));
    // Send command
    outb(ATA_PORT_COMMAND, ATA_CMD_READ);

    // Wait for drive to process and provide data
    ata_wait_bsy();
    ata_wait_drq();

    // Read 256 16-bit words (512 bytes)
    uint16_t* ptr = (uint16_t*)buf;
    for (int i = 0; i < 256; i++) {
        ptr[i] = inw(ATA_PORT_DATA);
    }
}

void ata_write_sector(uint32_t lba, const uint8_t* buf) {
    ata_wait_bsy();

    // Select drive
    outb(ATA_PORT_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    // Number of sectors to write
    outb(ATA_PORT_SEC_COUNT, 1);
    // Send LBA bits
    outb(ATA_PORT_LBA_LOW, (uint8_t)lba);
    outb(ATA_PORT_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PORT_LBA_HIGH, (uint8_t)(lba >> 16));
    // Send command
    outb(ATA_PORT_COMMAND, ATA_CMD_WRITE);

    // Wait for drive to prepare to receive data
    ata_wait_bsy();
    ata_wait_drq();

    // Write 256 16-bit words (512 bytes)
    uint16_t* ptr = (uint16_t*)buf;
    for (int i = 0; i < 256; i++) {
        outw(ATA_PORT_DATA, ptr[i]);
    }

    // Flush cache
    outb(ATA_PORT_COMMAND, ATA_CMD_CACHE_FLUSH);
    ata_wait_bsy();
}
