#include "tty.h"
#include "vga.h"
#include "timer.h"

/*
 * SentinelOS TTY Driver
 * ─────────────────────
 * Owns rows TTY_TOP_ROW..TTY_BOT_ROW (rows 1-18) as a scrollable text
 * terminal. Text is cyan on black; borders are managed by the dashboard.
 *
 * Internal shadow buffer keeps a copy of displayed characters so we can
 * scroll without re-reading from VGA memory (which is write-only-friendly).
 */

/* Shadow buffer: TTY_ROWS × TTY_COLS characters */
static char shadow[TTY_ROWS][TTY_COLS];
static uint8_t shadow_attr[TTY_ROWS][TTY_COLS];
static int  tty_col = 0;    /* current cursor column (0-based, relative) */
static int  tty_row = 0;    /* current cursor row    (0-based, relative) */
static uint8_t current_style = STYLE_DEFAULT;


/* ── Internal helpers ────────────────────────────────────────────────────── */

/* Flush entire shadow buffer to VGA */
static void tty_flush_all(void) {
    for (int r = 0; r < TTY_ROWS; r++)
        for (int c = 0; c < TTY_COLS; c++)
            vga_putchar(TTY_LEFT_COL + c, TTY_TOP_ROW + r,
                        shadow[r][c], shadow_attr[r][c]);
}

/* Scroll up by one line */
static void tty_scroll(void) {
    /* Shift shadow rows up */
    for (int r = 0; r < TTY_ROWS - 1; r++) {
        for (int c = 0; c < TTY_COLS; c++) {
            shadow[r][c] = shadow[r + 1][c];
            shadow_attr[r][c] = shadow_attr[r + 1][c];
        }
    }
    /* Clear last row */
    for (int c = 0; c < TTY_COLS; c++) {
        shadow[TTY_ROWS - 1][c] = ' ';
        shadow_attr[TTY_ROWS - 1][c] = current_style;
    }
    tty_flush_all();
    tty_row = TTY_ROWS - 1;
}

/* Draw cursor block at current position */
static void tty_draw_cursor(void) {
    int scr_x = TTY_LEFT_COL + tty_col;
    int scr_y = TTY_TOP_ROW  + tty_row;
    /* Draw underscore cursor using reverse-video on the current char */
    char ch = shadow[tty_row][tty_col];
    vga_putchar(scr_x, scr_y, ch ? ch : ' ',
                VGA_ATTR(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_CYAN));
}

static void tty_erase_cursor(void) {
    int scr_x = TTY_LEFT_COL + tty_col;
    int scr_y = TTY_TOP_ROW  + tty_row;
    char ch = shadow[tty_row][tty_col];
    vga_putchar(scr_x, scr_y, ch ? ch : ' ', shadow_attr[tty_row][tty_col]);
}

void tty_set_color(uint8_t fg, uint8_t bg) {
    current_style = VGA_ATTR(bg, fg);
}

void tty_reset_color(void) {
    current_style = STYLE_DEFAULT;
}

/* ── Public API ──────────────────────────────────────────────────────────── */

void tty_clear(void) {
    for (int r = 0; r < TTY_ROWS; r++) {
        for (int c = 0; c < TTY_COLS; c++) {
            shadow[r][c] = ' ';
            shadow_attr[r][c] = current_style;
        }
    }
    tty_flush_all();
    tty_col = 0;
    tty_row = 0;
}

void tty_init(void) {
    tty_clear();
    /* Welcome banner — uses CP437 box chars as hex escapes:
     * \xC9=╔  \xBB=╗  \xC8=╚  \xBC=╝  \xCD=═  \xBA=║ */
    tty_puts("  \xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB\n");
    tty_puts("  \xBA        Welcome to SentinelOS v0.1            \xBA\n");
    tty_puts("  \xBA  Security-first microkernel  //  Ring 0      \xBA\n");
    tty_puts("  \xC8\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC\n");
    tty_puts("\n");
    tty_puts("  Type 'help' for available commands.\n\n");
}


void tty_putchar(char c) {
    tty_erase_cursor();

    if (c == '\n' || c == '\r') {
        /* Fill rest of row with spaces */
        for (int col = tty_col; col < TTY_COLS; col++)
            shadow[tty_row][col] = ' ';
        tty_col = 0;
        tty_row++;
        if (tty_row >= TTY_ROWS) tty_scroll();
        tty_draw_cursor();
        return;
    }

    if (c == '\t') {
        /* Tab → advance to next 4-char boundary */
        int next = (tty_col + 4) & ~3;
        if (next >= TTY_COLS) next = TTY_COLS - 1;
        for (int col = tty_col; col < next; col++) {
            shadow[tty_row][col] = ' ';
            shadow_attr[tty_row][col] = current_style;
            vga_putchar(TTY_LEFT_COL + col, TTY_TOP_ROW + tty_row, ' ', current_style);
        }
        tty_col = next;
        tty_draw_cursor();
        return;
    }

    /* Normal printable character */
    shadow[tty_row][tty_col] = c;
    shadow_attr[tty_row][tty_col] = current_style;
    vga_putchar(TTY_LEFT_COL + tty_col, TTY_TOP_ROW + tty_row, c, current_style);
    tty_col++;
    if (tty_col >= TTY_COLS) {
        tty_col = 0;
        tty_row++;
        if (tty_row >= TTY_ROWS) tty_scroll();
    }
    tty_draw_cursor();
}

void tty_puts(const char* s) {
    for (int i = 0; s[i]; i++) tty_putchar(s[i]);
}

void tty_put_uint(uint32_t v) {
    char buf[12]; int i = 11;
    buf[i] = '\0';
    if (v == 0) { buf[--i] = '0'; }
    else { while (v > 0) { buf[--i] = '0' + (v % 10); v /= 10; } }
    tty_puts(&buf[i]);
}

void tty_put_hex(uint32_t v) {
    static const char h[] = "0123456789ABCDEF";
    tty_puts("0x");
    for (int i = 7; i >= 0; i--)
        tty_putchar(h[(v >> (i * 4)) & 0xF]);
}
