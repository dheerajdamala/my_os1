#include "vga.h"
#include "io.h"

/* ── Helpers ────────────────────────────────────────────────────────────── */

static inline void vga_write(int x, int y, char ch, uint8_t attr) {
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) return;
    VGA_MEMORY[y * VGA_WIDTH + x] = VGA_ENTRY(ch, attr);
}

/* ── Public API ─────────────────────────────────────────────────────────── */

void vga_hide_cursor(void) {
    /* Disable the hardware blinking cursor via VGA CRT controller */
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void vga_clear(uint8_t attr) {
    for (int y = 0; y < VGA_HEIGHT; y++)
        for (int x = 0; x < VGA_WIDTH; x++)
            vga_write(x, y, ' ', attr);
}

void vga_init(void) {
    vga_hide_cursor();
    vga_clear(STYLE_DEFAULT);
}

void vga_putchar(int x, int y, char ch, uint8_t attr) {
    vga_write(x, y, ch, attr);
}

void vga_puts(int x, int y, const char* str, uint8_t attr) {
    for (int i = 0; str[i] != '\0'; i++)
        vga_write(x + i, y, str[i], attr);
}

void vga_put_uint(int x, int y, uint32_t val, uint8_t attr) {
    char buf[12];
    int i = 11;
    buf[i] = '\0';
    if (val == 0) { buf[--i] = '0'; }
    else { while (val > 0) { buf[--i] = '0' + (val % 10); val /= 10; } }
    vga_puts(x, y, &buf[i], attr);
}

/* Right-aligned, zero-padded number in a field of `width` chars */
void vga_put_uint_padded(int x, int y, uint32_t val, int width, uint8_t attr) {
    char buf[12];
    int i = 11;
    buf[i] = '\0';
    if (val == 0) { buf[--i] = '0'; }
    else { while (val > 0) { buf[--i] = '0' + (val % 10); val /= 10; } }
    int len = 11 - i;
    int pad = width - len;
    for (int p = 0; p < pad; p++) vga_write(x + p, y, '0', attr);
    vga_puts(x + pad, y, &buf[i], attr);
}

void vga_fill_rect(int x, int y, int w, int h, char ch, uint8_t attr) {
    for (int row = y; row < y + h; row++)
        for (int col = x; col < x + w; col++)
            vga_write(col, row, ch, attr);
}

void vga_draw_hline(int x, int y, int w, uint8_t attr) {
    for (int col = x; col < x + w; col++)
        vga_write(col, y, '\xCD', attr); /* ═ double-horizontal */
}

/* Box-drawing using IBM CP437 characters available in VGA text mode:
 * \xC9 = ╔  \xBB = ╗  \xC8 = ╚  \xBC = ╝
 * \xCD = ═  \xBA = ║  \xCC = ╠  \xB9 = ╣  */
void vga_draw_box(int x, int y, int w, int h, uint8_t attr) {
    /* Corners */
    vga_write(x,         y,         '\xC9', attr); /* ╔ */
    vga_write(x + w - 1, y,         '\xBB', attr); /* ╗ */
    vga_write(x,         y + h - 1, '\xC8', attr); /* ╚ */
    vga_write(x + w - 1, y + h - 1, '\xBC', attr); /* ╝ */
    /* Top / bottom edges */
    for (int col = x + 1; col < x + w - 1; col++) {
        vga_write(col, y,         '\xCD', attr); /* ═ */
        vga_write(col, y + h - 1, '\xCD', attr); /* ═ */
    }
    /* Left / right edges */
    for (int row = y + 1; row < y + h - 1; row++) {
        vga_write(x,         row, '\xBA', attr); /* ║ */
        vga_write(x + w - 1, row, '\xBA', attr); /* ║ */
    }
}
