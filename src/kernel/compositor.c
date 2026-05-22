#include "compositor.h"
#include "vbe.h"
#include "vga.h"
#include "mouse.h"
#include "timer.h"
#include "scheduler.h"
#include "serial.h"

window_t windows[2];

// Focus window index (0 or 1)
static int active_win_idx = 0;

static void compositor_sleep(uint32_t ticks) {
    uint32_t start = timer_get_ticks();
    while (timer_get_ticks() - start < ticks) {
        scheduler_yield();
    }
}

static void draw_wallpaper(void) {
    // Premium dark space cadet background
    vbe_clear(0x0F0F11);
    
    // Draw high-tech grid dots
    for (uint32_t y = 40; y < vbe_height; y += 40) {
        for (uint32_t x = 20; x < vbe_width; x += 40) {
            vbe_put_pixel(x, y, 0x1E293B); // Slate dot
        }
    }
}

static void draw_cursor(int mx, int my) {
    // Modern cursor with black border and white body
    static const char cursor_bitmap[12][13] = {
        "X           ",
        "XX          ",
        "XXX         ",
        "XXXX        ",
        "XXXXX       ",
        "XXXXXX      ",
        "XXXXXXX     ",
        "XXXXXXXX    ",
        "XXXXX       ",
        "XX  XX      ",
        "X    XX     ",
        "      XX    "
    };
    
    for (int r = 0; r < 12; r++) {
        for (int c = 0; c < 12; c++) {
            if (mx + c >= (int)vbe_width || my + r >= (int)vbe_height) continue;
            if (cursor_bitmap[r][c] == 'X') {
                vbe_put_pixel(mx + c, my + r, 0xFFFFFF); // White
            } else if (cursor_bitmap[r][c] == ' ') {
                vbe_put_pixel(mx + c, my + r, 0x000000); // Black border
            }
        }
    }
}

static void draw_box_lines_8x16(uint8_t ch, uint32_t px, uint32_t py, uint32_t fg, uint32_t bg) {
    vbe_draw_rect(px, py, 8, 16, bg);
    switch (ch) {
        case 0xCD: // ═
            vbe_draw_rect(px, py + 6, 8, 1, fg);
            vbe_draw_rect(px, py + 10, 8, 1, fg);
            break;
        case 0xBA: // ║
            vbe_draw_rect(px + 2, py, 1, 16, fg);
            vbe_draw_rect(px + 5, py, 1, 16, fg);
            break;
        case 0xC9: // ╔
            vbe_draw_rect(px + 2, py + 6, 6, 1, fg);
            vbe_draw_rect(px + 5, py + 10, 3, 1, fg);
            vbe_draw_rect(px + 2, py + 6, 1, 10, fg);
            vbe_draw_rect(px + 5, py + 10, 1, 6, fg);
            break;
        case 0xBB: // ╗
            vbe_draw_rect(px, py + 6, 3, 1, fg);
            vbe_draw_rect(px, py + 10, 6, 1, fg);
            vbe_draw_rect(px + 2, py + 10, 1, 6, fg);
            vbe_draw_rect(px + 5, py + 6, 1, 10, fg);
            break;
        case 0xC8: // ╚
            vbe_draw_rect(px + 2, py, 1, 7, fg);
            vbe_draw_rect(px + 5, py, 1, 3, fg);
            vbe_draw_rect(px + 2, py + 6, 6, 1, fg);
            vbe_draw_rect(px + 5, py + 10, 3, 1, fg);
            break;
        case 0xBC: // ╝
            vbe_draw_rect(px + 2, py, 1, 3, fg);
            vbe_draw_rect(px + 5, py, 1, 7, fg);
            vbe_draw_rect(px, py + 10, 3, 1, fg);
            vbe_draw_rect(px, py + 6, 6, 1, fg);
            break;
        case 0xCC: // ╠
            vbe_draw_rect(px + 2, py, 1, 16, fg);
            vbe_draw_rect(px + 5, py, 1, 16, fg);
            vbe_draw_rect(px + 5, py + 6, 3, 1, fg);
            vbe_draw_rect(px + 5, py + 10, 3, 1, fg);
            break;
        case 0xB9: // ╣
            vbe_draw_rect(px + 2, py, 1, 16, fg);
            vbe_draw_rect(px + 5, py, 1, 16, fg);
            vbe_draw_rect(px, py + 6, 3, 1, fg);
            vbe_draw_rect(px, py + 10, 3, 1, fg);
            break;
        case 0xDB: // █
            vbe_draw_rect(px, py, 8, 16, fg);
            break;
        case 0xB0: // ░ light shaded block
            vbe_draw_rect(px, py, 8, 16, bg);
            for (int r = 0; r < 16; r += 4) {
                for (int c = 0; c < 8; c += 4) {
                    vbe_put_pixel(px + c, py + r, fg);
                }
            }
            break;
        case 0xB1: // ▒ medium shaded block
            vbe_draw_rect(px, py, 8, 16, bg);
            for (int r = 0; r < 16; r += 2) {
                for (int c = 0; c < 8; c += 2) {
                    if ((r + c) % 4 == 0) vbe_put_pixel(px + c, py + r, fg);
                }
            }
            break;
        case 0xB2: // ▓ dark shaded block
            vbe_draw_rect(px, py, 8, 16, bg);
            for (int r = 0; r < 16; r++) {
                for (int c = 0; c < 8; c++) {
                    if ((r + c) % 2 == 0) vbe_put_pixel(px + c, py + r, fg);
                }
            }
            break;
    }
}

static void draw_window(window_t* win) {
    // 1. Draw outer border
    vbe_draw_rect(win->x, win->y, win->w, win->h, 0x1F2937); // Slate-800 border
    vbe_draw_rect(win->x + 1, win->y + 1, win->w - 2, win->h - 2, 0x374151); // Secondary border

    // 2. Draw Title bar
    vbe_draw_rect(win->x + 2, win->y + 2, win->w - 4, 22, 0x0F172A); // Title bar fill

    // Active focused window title bar styling
    if (win->active) {
        vbe_draw_rect(win->x + 2, win->y + 23, win->w - 4, 1, 0x06B6D4); // Neon Cyan highlight
        vbe_draw_string(win->title, win->x + 12, win->y + 5, 0x22D3EE, 0x0F172A); // Cyan text
    } else {
        vbe_draw_rect(win->x + 2, win->y + 23, win->w - 4, 1, 0x374151); // Gray divider
        vbe_draw_string(win->title, win->x + 12, win->y + 5, 0x9CA3AF, 0x0F172A); // Muted gray text
    }

    // 3. Draw Close/Min/Max macOS style buttons
    vbe_draw_rect(win->x + win->w - 55, win->y + 8, 8, 8, 0xEF4444); // Red
    vbe_draw_rect(win->x + win->w - 40, win->y + 8, 8, 8, 0xF59E0B); // Yellow
    vbe_draw_rect(win->x + win->w - 25, win->y + 8, 8, 8, 0x10B981); // Green

    // 4. Draw Client area
    vbe_draw_rect(win->x + 8, win->y + 24, win->client_w, win->client_h, 0x0A0F1D); // Dark obsidian

    // 5. Draw Client content character-by-character
    for (int r = 0; r < win->row_count; r++) {
        int vga_row = win->row_start + r;
        for (int c = 0; c < 80; c++) {
            char ch = vga_char_buf[vga_row][c];
            uint8_t attr = vga_attr_buf[vga_row][c];
            uint32_t fg = vga_to_rgb(attr & 0x0F);
            uint32_t bg = vga_to_rgb(attr >> 4);

            if (bg == 0x121214) { // Transparent mapping for default VGA background color
                bg = 0x0A0F1D;
            }

            uint32_t px = win->x + 8 + c * 8;
            uint32_t py = win->y + 24 + r * 16;

            if ((uint8_t)ch >= 0x80) {
                draw_box_lines_8x16((uint8_t)ch, px, py, fg, bg);
            } else {
                vbe_draw_rect(px, py, 8, 16, bg);
                if (ch != ' ' && ch != '\0') {
                    vbe_draw_char(ch, px, py, fg, bg);
                }
            }
        }
    }
}

void compositor_init(void) {
    serial_printf("[COMPOSITOR] Setting up windows...\n");
    
    // Window 0: Terminal Window
    windows[0].client_w = 640;
    windows[0].client_h = 304; // 19 rows * 16 pixels
    windows[0].w = windows[0].client_w + 16;
    windows[0].h = windows[0].client_h + 32;
    windows[0].x = 40;
    windows[0].y = 60;
    windows[0].title = "Sentinel Terminal Console";
    windows[0].row_start = 1;
    windows[0].row_count = 19;
    windows[0].active = 0;

    // Window 1: System Monitor Window
    windows[1].client_w = 640;
    windows[1].client_h = 80; // 5 rows * 16 pixels
    windows[1].w = windows[1].client_w + 16;
    windows[1].h = windows[1].client_h + 32;
    windows[1].x = 40;
    windows[1].y = 420;
    windows[1].title = "Sentinel System Monitor";
    windows[1].row_start = 20;
    windows[1].row_count = 5;
    windows[1].active = 1;

    active_win_idx = 1; // System Monitor starts as focused (drawn on top)
}

void compositor_loop(void) {
    serial_printf("[COMPOSITOR] Starting compositor loop...\n");
    compositor_active = 1;

    int dragging = -1;
    int drag_off_x = 0;
    int drag_off_y = 0;

    while (1) {
        // Drag-and-drop & focus window detection
        if (mouse_left_click) {
            if (dragging == -1) {
                // Check if cursor clicked on any window title bar (from topmost to bottommost)
                for (int i = 1; i >= 0; i--) {
                    window_t* win = &windows[i];
                    if (mouse_x >= win->x && mouse_x < win->x + win->w &&
                        mouse_y >= win->y && mouse_y < win->y + 24) {
                        dragging = i;
                        drag_off_x = mouse_x - win->x;
                        drag_off_y = mouse_y - win->y;

                        // Focus shift: Bring clicked window to the front (index 1)
                        if (i == 0) {
                            window_t temp = windows[0];
                            windows[0] = windows[1];
                            windows[1] = temp;
                            windows[0].active = 0;
                            windows[1].active = 1;
                            dragging = 1;
                            active_win_idx = 1;
                        }
                        break;
                    }
                }
            } else {
                // Drag the currently active window
                window_t* win = &windows[dragging];
                win->x = mouse_x - drag_off_x;
                win->y = mouse_y - drag_off_y;

                // Restrict boundaries so the window title bar is always visible
                if (win->x < -win->w + 50) win->x = -win->w + 50;
                if (win->x >= (int)vbe_width - 50) win->x = vbe_width - 50;
                if (win->y < 24) win->y = 24; // Don't cover top bar
                if (win->y >= (int)vbe_height - 24) win->y = vbe_height - 24;
            }
        } else {
            dragging = -1;
        }

        // Draw wallpaper
        draw_wallpaper();

        // Draw top bar
        vbe_draw_rect(0, 0, 1024, 24, 0x13151A); // Dark Top Bar
        vbe_draw_rect(0, 24, 1024, 1, 0x1E293B); // Dark border separating top bar
        
        for (int x = 0; x < 80; x++) {
            char ch = vga_char_buf[0][x];
            uint8_t attr = vga_attr_buf[0][x];
            uint32_t fg = vga_to_rgb(attr & 0x0F);
            uint32_t bg = vga_to_rgb(attr >> 4);
            
            if (bg == 0x121214) {
                bg = 0x13151A; // Transparent mapping for top bar
            }

            int px = 192 + x * 8; // Centered
            int py = 4;
            
            if ((uint8_t)ch >= 0x80) {
                draw_box_lines_8x16((uint8_t)ch, px, py, fg, bg);
            } else {
                vbe_draw_rect(px, py, 8, 16, bg);
                if (ch != ' ' && ch != '\0') {
                    vbe_draw_char(ch, px, py, fg, bg);
                }
            }
        }

        // Render windows (windows[0] under windows[1])
        draw_window(&windows[0]);
        draw_window(&windows[1]);

        // Draw mouse pointer
        draw_cursor(mouse_x, mouse_y);

        // Swap to screen
        vbe_swap_buffers();

        // 30 FPS sleep limit
        compositor_sleep(3);
    }
}
