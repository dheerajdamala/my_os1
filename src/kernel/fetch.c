#include "fetch.h"
#include "tty.h"
#include "vga.h"
#include "memory.h"
#include "scheduler.h"
#include "timer.h"

void cmd_fetch(void) {
    tty_puts("\n");
    
    /* Neofetch style output */
    /* Column 1: Logo (Cyan) */
    /* Column 2: Stats */
    
    /* Row 1 */
    tty_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    tty_puts("   \xDC\xDC\xDB\xDB\xDB\xDC\xDC   ");
    tty_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
    tty_puts("  dheeraj");
    tty_reset_color();
    tty_puts("@");
    tty_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    tty_puts("sentinel\n");
    
    /* Row 2 */
    tty_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    tty_puts(" \xDC\xDB\xDF   \xDF\xDB\xDC  ");
    tty_reset_color();
    tty_puts("  \xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\n");
    
    /* Row 3 */
    tty_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    tty_puts(" \xDB\xDF \xDC\xDB\xDC \xDF\xDB  ");
    tty_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    tty_puts("  OS     : ");
    tty_reset_color();
    tty_puts("SentinelOS v0.1.0\n");

    /* Row 4 */
    tty_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    tty_puts(" \xDB \xDB\xDF \xDF\xDB \xDB  ");
    tty_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    tty_puts("  Kernel : ");
    tty_reset_color();
    tty_puts("Microkernel (Ring 0)\n");

    /* Row 5 */
    tty_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    tty_puts(" \xDB \xDB\xDC \xDC\xDB \xDB  ");
    tty_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    tty_puts("  Uptime : ");
    tty_reset_color();
    uint32_t ticks = timer_get_ticks();
    uint32_t s = ticks / 100;
    tty_put_uint(s / 60); tty_puts(" mins, "); tty_put_uint(s % 60); tty_puts(" secs\n");

    /* Row 6 */
    tty_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    tty_puts(" \xDB\xDC \xDF\xDB\xDF \xDC\xDB  ");
    tty_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    tty_puts("  Threads: ");
    tty_reset_color();
    tty_put_uint(get_thread_count()); tty_puts("\n");

    /* Row 7 */
    tty_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    tty_puts(" \xDF\xDB\xDC   \xDC\xDB\xDF  ");
    tty_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    tty_puts("  Memory : ");
    tty_reset_color();
    uint32_t mem_used = kheap_used() / 1024;
    tty_put_uint(mem_used); tty_puts(" KB / 40 KB\n");

    /* Row 8 */
    tty_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    tty_puts("   \xDF\xDF\xDB\xDB\xDB\xDF\xDF   ");
    tty_puts("  \n");
    
    /* Row 9 (Color blocks) */
    tty_puts("              ");
    for (int i=0; i<8; i++) {
        tty_set_color(i, i);
        tty_puts("   ");
    }
    tty_reset_color();
    tty_puts("\n");
    
    tty_puts("              ");
    for (int i=8; i<16; i++) {
        tty_set_color(i, i);
        tty_puts("   ");
    }
    tty_reset_color();
    tty_puts("\n\n");
}
