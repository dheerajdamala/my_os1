#include "matrix.h"
#include "tty.h"
#include "vga.h"
#include "timer.h"
#include "keyboard.h"

/* Simple PRNG */
static uint32_t seed = 0x12345678;
static uint32_t rand(void) {
    seed = (1103515245 * seed + 12345);
    return (seed >> 16) & 0x7FFF;
}

#define NUM_DROPS TTY_COLS

void cmd_matrix(void) {
    tty_clear();
    seed += timer_get_ticks(); /* randomize a bit */
    
    int drop_y[NUM_DROPS];
    int drop_speed[NUM_DROPS];
    
    for (int i=0; i<NUM_DROPS; i++) {
        drop_y[i] = -(int)(rand() % TTY_ROWS);
        drop_speed[i] = 1 + (rand() % 3);
    }
    
    /* We write directly to VGA to avoid scrolling the shadow buffer in tty.c */
    while (1) {
        if (keyboard_getchar() != 0) break;
        
        for (int i=0; i<NUM_DROPS; i++) {
            /* Erase old trail (a bit above the drop) */
            int tail_y = drop_y[i] - 5 - (rand() % 4);
            if (tail_y >= 0 && tail_y < TTY_ROWS) {
                vga_putchar(TTY_LEFT_COL + i, TTY_TOP_ROW + tail_y, ' ', STYLE_DEFAULT);
            }
            
            /* Move drop */
            if (timer_get_ticks() % drop_speed[i] == 0) {
                drop_y[i]++;
            }
            
            if (drop_y[i] >= TTY_ROWS + 10) {
                drop_y[i] = -(int)(rand() % 5);
                drop_speed[i] = 1 + (rand() % 3);
            }
            
            /* Draw drop */
            if (drop_y[i] >= 0 && drop_y[i] < TTY_ROWS) {
                /* Head is white, body is green */
                char c = 33 + (rand() % 94); /* Random ASCII */
                vga_putchar(TTY_LEFT_COL + i, TTY_TOP_ROW + drop_y[i], c, VGA_ATTR(VGA_COLOR_BLACK, VGA_COLOR_WHITE));
                
                /* Body */
                if (drop_y[i] - 1 >= 0) {
                    vga_putchar(TTY_LEFT_COL + i, TTY_TOP_ROW + drop_y[i] - 1, c, VGA_ATTR(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREEN));
                }
                /* Dark body */
                if (drop_y[i] - 3 >= 0) {
                    vga_putchar(TTY_LEFT_COL + i, TTY_TOP_ROW + drop_y[i] - 3, c, VGA_ATTR(VGA_COLOR_BLACK, VGA_COLOR_GREEN));
                }
            }
        }
        
        /* Delay loop using timer tick */
        uint32_t start = timer_get_ticks();
        while (timer_get_ticks() - start < 5);
    }
    
    tty_clear();
}
