#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include <stddef.h>

/* VGA text mode dimensions */
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

/* VGA color codes (4-bit) */
#define VGA_COLOR_BLACK         0x0
#define VGA_COLOR_BLUE          0x1
#define VGA_COLOR_GREEN         0x2
#define VGA_COLOR_CYAN          0x3
#define VGA_COLOR_RED           0x4
#define VGA_COLOR_MAGENTA       0x5
#define VGA_COLOR_BROWN         0x6
#define VGA_COLOR_LIGHT_GREY    0x7
#define VGA_COLOR_DARK_GREY     0x8
#define VGA_COLOR_LIGHT_BLUE    0x9
#define VGA_COLOR_LIGHT_GREEN   0xA
#define VGA_COLOR_LIGHT_CYAN    0xB
#define VGA_COLOR_LIGHT_RED     0xC
#define VGA_COLOR_LIGHT_MAGENTA 0xD
#define VGA_COLOR_YELLOW        0xE
#define VGA_COLOR_WHITE         0xF

/* Combine fg + bg into a single attribute byte */
#define VGA_ATTR(fg, bg)  ((uint8_t)((bg) << 4 | (fg)))

/* Combine char + attribute into a VGA cell */
#define VGA_ENTRY(ch, attr) ((uint16_t)((attr) << 8 | (uint8_t)(ch)))

/* Named palette presets for the UI */
#define STYLE_DEFAULT   VGA_ATTR(VGA_COLOR_LIGHT_CYAN,    VGA_COLOR_BLACK)
#define STYLE_BORDER    VGA_ATTR(VGA_COLOR_CYAN,          VGA_COLOR_BLACK)
#define STYLE_ACCENT    VGA_ATTR(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK)
#define STYLE_LABEL     VGA_ATTR(VGA_COLOR_DARK_GREY,     VGA_COLOR_BLACK)
#define STYLE_VALUE     VGA_ATTR(VGA_COLOR_WHITE,         VGA_COLOR_BLACK)
#define STYLE_BAR_FILL  VGA_ATTR(VGA_COLOR_LIGHT_CYAN,   VGA_COLOR_BLACK)
#define STYLE_BAR_EMPTY VGA_ATTR(VGA_COLOR_DARK_GREY,    VGA_COLOR_BLACK)
#define STYLE_LOGO      VGA_ATTR(VGA_COLOR_LIGHT_CYAN,   VGA_COLOR_BLACK)
#define STYLE_TAGLINE   VGA_ATTR(VGA_COLOR_LIGHT_MAGENTA,VGA_COLOR_BLACK)
#define STYLE_SPLASH_BG VGA_ATTR(VGA_COLOR_BLACK,        VGA_COLOR_BLACK)
#define STYLE_HEADER    VGA_ATTR(VGA_COLOR_YELLOW,        VGA_COLOR_BLACK)

extern char vga_char_buf[25][80];
extern uint8_t vga_attr_buf[25][80];
extern int compositor_active;
uint32_t vga_to_rgb(uint8_t color_index);
void draw_box_lines(uint8_t ch, uint32_t px, uint32_t py, uint32_t fg, uint32_t bg);

/* Core API */
void vga_init(void);
void vga_clear(uint8_t attr);
void vga_putchar(int x, int y, char ch, uint8_t attr);
void vga_puts(int x, int y, const char* str, uint8_t attr);
void vga_put_uint(int x, int y, uint32_t val, uint8_t attr);
void vga_put_uint_padded(int x, int y, uint32_t val, int width, uint8_t attr);
void vga_fill_rect(int x, int y, int w, int h, char ch, uint8_t attr);
void vga_draw_hline(int x, int y, int w, uint8_t attr);
void vga_draw_box(int x, int y, int w, int h, uint8_t attr);
void vga_hide_cursor(void);

#endif /* VGA_H */
