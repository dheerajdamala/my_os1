#include "mouse.h"
#include "idt.h"
#include "io.h"
#include "vbe.h"
#include "serial.h"

int mouse_x = 512;
int mouse_y = 384;
int mouse_left_click = 0;
int mouse_right_click = 0;
int mouse_updated = 0;

static uint8_t mouse_cycle = 0;
static uint8_t mouse_byte[3];

static inline void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if (inb(0x64) & 1) return;
        }
    } else {
        while (timeout--) {
            if (!(inb(0x64) & 2)) return;
        }
    }
}

static inline void mouse_write(uint8_t write_to_device, uint8_t data) {
    if (write_to_device) {
        mouse_wait(1);
        outb(0x64, 0xD4);
    }
    mouse_wait(1);
    outb(0x60, data);
}

static inline uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

static void mouse_handler(registers_t* regs) {
    (void)regs;
    uint8_t status = inb(0x64);
    if (!(status & 1)) return;      // No data in keyboard buffer
    if (!(status & 0x20)) return;   // Data is not from mouse

    mouse_byte[mouse_cycle++] = inb(0x60);

    if (mouse_cycle == 3) {
        mouse_cycle = 0;

        // Verify sync bit (bit 3 of byte 0 should always be 1)
        if (!(mouse_byte[0] & 0x08)) {
            return;
        }

        int x_rel = (int)mouse_byte[1];
        int y_rel = (int)mouse_byte[2];

        // Sign extend relative coordinate offsets if sign bits are set
        if (mouse_byte[0] & 0x10) x_rel |= 0xFFFFFF00;
        if (mouse_byte[0] & 0x20) y_rel |= 0xFFFFFF00;

        // Button clicks
        int left = mouse_byte[0] & 0x01;
        int right = (mouse_byte[0] & 0x02) >> 1;

        mouse_x += x_rel;
        mouse_y -= y_rel; // Y motion is inverted in PS/2 relative to standard pixel coordinates

        // Clamp inside screen bounds
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x >= (int)vbe_width) mouse_x = vbe_width - 1;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y >= (int)vbe_height) mouse_y = vbe_height - 1;

        mouse_left_click = left;
        mouse_right_click = right;
        mouse_updated = 1;
    }
}

void mouse_init(void) {
    serial_printf("[MOUSE] Initializing PS/2 mouse controller...\n");

    // Enable auxiliary mouse device
    mouse_wait(1);
    outb(0x64, 0xA8);

    // Read controller command byte
    mouse_wait(1);
    outb(0x64, 0x20);
    uint8_t status = mouse_read();

    // Enable mouse interrupt (bit 1) and clear disable mouse (bit 5)
    status |= 2;
    status &= ~0x20;

    // Write modified command byte
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);

    // Tell the mouse auxiliary device to use default settings
    mouse_write(1, 0xF6);
    mouse_read(); // Acknowledge byte (0xFA)

    // Enable data packet streaming
    mouse_write(1, 0xF4);
    mouse_read(); // Acknowledge byte (0xFA)

    // Register handler for IRQ 12
    register_interrupt_handler(44, mouse_handler);

    mouse_x = vbe_width / 2;
    mouse_y = vbe_height / 2;
    mouse_left_click = 0;
    mouse_right_click = 0;
    mouse_updated = 0;

    serial_printf("[MOUSE] Mouse successfully configured on IRQ 12.\n");
}
