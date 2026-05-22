#include "vga.h"
#include "vbe.h"
#include "io.h"

int compositor_active = 0;

// Virtual VGA character and attribute buffers for compositor lookup
char vga_char_buf[25][80];
uint8_t vga_attr_buf[25][80];

/* ── Color Helper ───────────────────────────────────────────────────────── */

uint32_t vga_to_rgb(uint8_t color_index) {
    switch (color_index & 0x0F) {
        case 0x0: return 0x121214; // Sleek Dark Space Cadet
        case 0x1: return 0x1E3A8A; // Dark Blue
        case 0x2: return 0x065F46; // Dark Green
        case 0x3: return 0x0891B2; // Teal/Cyan
        case 0x4: return 0x991B1B; // Dark Red
        case 0x5: return 0x86198F; // Magenta
        case 0x6: return 0x78350F; // Brown
        case 0x7: return 0xD1D5DB; // Light Gray
        case 0x8: return 0x374151; // Dark Gray
        case 0x9: return 0x3B82F6; // Blue
        case 0xA: return 0x10B981; // Emerald/Mint Green
        case 0xB: return 0x06B6D4; // Neon Cyan
        case 0xC: return 0xEF4444; // Neon Red
        case 0xD: return 0xD946EF; // Bright Magenta
        case 0xE: return 0xFBBF24; // Yellow
        case 0xF: return 0xF9FAFB; // White
        default: return 0x000000;
    }
}

/* ── Box Drawing Helper ─────────────────────────────────────────────────── */

void draw_box_lines(uint8_t ch, uint32_t px, uint32_t py, uint32_t fg, uint32_t bg) {
    vbe_draw_rect(px, py, 12, 24, bg);

    switch (ch) {
        case 0xCD: // ═ double horizontal
            vbe_draw_rect(px, py + 9, 12, 2, fg);
            vbe_draw_rect(px, py + 14, 12, 2, fg);
            break;
        case 0xBA: // ║ double vertical
            vbe_draw_rect(px + 4, py, 2, 24, fg);
            vbe_draw_rect(px + 7, py, 2, 24, fg);
            break;
        case 0xC9: // ╔ top-left corner
            vbe_draw_rect(px + 4, py + 9, 8, 2, fg);
            vbe_draw_rect(px + 7, py + 14, 5, 2, fg);
            vbe_draw_rect(px + 4, py + 9, 2, 15, fg);
            vbe_draw_rect(px + 7, py + 14, 2, 10, fg);
            break;
        case 0xBB: // ╗ top-right corner
            vbe_draw_rect(px, py + 9, 5, 2, fg);
            vbe_draw_rect(px, py + 14, 8, 2, fg);
            vbe_draw_rect(px + 4, py + 14, 2, 10, fg);
            vbe_draw_rect(px + 7, py + 9, 2, 15, fg);
            break;
        case 0xC8: // ╚ bottom-left corner
            vbe_draw_rect(px + 4, py, 2, 10, fg);
            vbe_draw_rect(px + 7, py, 2, 5, fg);
            vbe_draw_rect(px + 4, py + 9, 8, 2, fg);
            vbe_draw_rect(px + 7, py + 14, 5, 2, fg);
            break;
        case 0xBC: // ╝ bottom-right corner
            vbe_draw_rect(px + 4, py, 2, 5, fg);
            vbe_draw_rect(px + 7, py, 2, 10, fg);
            vbe_draw_rect(px, py + 14, 5, 2, fg);
            vbe_draw_rect(px, py + 9, 8, 2, fg);
            break;
        case 0xCC: // ╠ left junction
            vbe_draw_rect(px + 4, py, 2, 24, fg);
            vbe_draw_rect(px + 7, py, 2, 24, fg);
            vbe_draw_rect(px + 7, py + 9, 5, 2, fg);
            vbe_draw_rect(px + 7, py + 14, 5, 2, fg);
            break;
        case 0xB9: // ╣ right junction
            vbe_draw_rect(px + 4, py, 2, 24, fg);
            vbe_draw_rect(px + 7, py, 2, 24, fg);
            vbe_draw_rect(px, py + 9, 5, 2, fg);
            vbe_draw_rect(px, py + 14, 5, 2, fg);
            break;
        case 0xDB: // █ solid block
            vbe_draw_rect(px, py, 12, 24, fg);
            break;
        case 0xB0: // ░ light shaded block
            vbe_draw_rect(px, py, 12, 24, bg);
            for (int r = 0; r < 24; r += 4) {
                for (int c = 0; c < 12; c += 4) {
                    vbe_put_pixel(px + c, py + r, fg);
                }
            }
            break;
        case 0xB1: // ▒ medium shaded block
            vbe_draw_rect(px, py, 12, 24, bg);
            for (int r = 0; r < 24; r += 2) {
                for (int c = 0; c < 12; c += 2) {
                    if ((r + c) % 4 == 0) {
                        vbe_put_pixel(px + c, py + r, fg);
                    }
                }
            }
            break;
        case 0xB2: // ▓ dark shaded block
            vbe_draw_rect(px, py, 12, 24, bg);
            for (int r = 0; r < 24; r++) {
                for (int c = 0; c < 12; c++) {
                    if ((r + c) % 2 == 0) {
                        vbe_put_pixel(px + c, py + r, fg);
                    }
                }
            }
            break;
    }
}

static inline void vga_write(int x, int y, char ch, uint8_t attr) {
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) return;
    
    // Save to the virtual buffer for compositor use
    vga_char_buf[y][x] = ch;
    vga_attr_buf[y][x] = attr;

    // Direct draw fallback only when compositor is inactive
    if (!compositor_active) {
        uint32_t px = x * 12 + 32;
        uint32_t py = y * 24 + 84;
        uint32_t fg = vga_to_rgb(attr & 0x0F);
        uint32_t bg = vga_to_rgb(attr >> 4);

        uint8_t u_ch = (uint8_t)ch;
        if (u_ch >= 0x80) {
            draw_box_lines(u_ch, px, py, fg, bg);
        } else {
            vbe_draw_rect(px, py, 12, 24, bg);
            if (ch != ' ' && ch != '\0') {
                vbe_draw_char(ch, px + 2, py + 4, fg, bg);
            }
        }
        vbe_copy_rect(px, py, 12, 24);
    }
}

/* ── Public API ─────────────────────────────────────────────────────────── */

void vga_hide_cursor(void) {}

void vga_clear(uint8_t attr) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_char_buf[y][x] = ' ';
            vga_attr_buf[y][x] = attr;
        }
    }

    if (!compositor_active) {
        uint32_t bg = vga_to_rgb(attr >> 4);
        vbe_clear(bg);
        vbe_swap_buffers();
    }
}

void vga_init(void) {
    vga_clear(VGA_ATTR(VGA_COLOR_BLACK, VGA_COLOR_BLACK));
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
        vga_write(col, y, '\xCD', attr);
}

void vga_draw_box(int x, int y, int w, int h, uint8_t attr) {
    vga_write(x,         y,         '\xC9', attr);
    vga_write(x + w - 1, y,         '\xBB', attr);
    vga_write(x,         y + h - 1, '\xC8', attr);
    vga_write(x + w - 1, y + h - 1, '\xBC', attr);
    for (int col = x + 1; col < x + w - 1; col++) {
        vga_write(col, y,         '\xCD', attr);
        vga_write(col, y + h - 1, '\xCD', attr);
    }
    for (int row = y + 1; row < y + h - 1; row++) {
        vga_write(x,         row, '\xBA', attr);
        vga_write(x + w - 1, row, '\xBA', attr);
    }
}
