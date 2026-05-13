#include "splash.h"
#include "vga.h"

/*
 * SentinelOS Boot Splash
 * ─────────────────────
 * Renders a full-screen cyberpunk aesthetic boot sequence directly to VGA
 * memory. No external dependencies — pure bare-metal rendering.
 *
 * Layout (80×25 VGA text mode):
 *   Rows  0-1  : top padding (black)
 *   Rows  2-8  : ASCII logo (7-line block font, bright cyan)
 *   Row   9    : blank
 *   Row  10    : tagline (centered, magenta)
 *   Row  11    : blank
 *   Row  12    : version string (centered, dark grey)
 *   Rows 13-19 : blank padding
 *   Row  20    : "INITIALIZING" label (centered, dark grey)
 *   Row  21    : loading bar (80 chars wide, cyan fill)
 *   Rows 22-24 : bottom padding
 */

/* ── Logo lines (7-line block-letter ASCII art, 72 chars wide) ─────────── */
static const char* LOGO[] = {
    " ::::::::: ::::::::::  ::::    ::: ::::::::::: ::::::::::: ::::    ::: :::::::::: :::        ",
    "  :+:    :+::+:        :+:+:   :+:     :+:         :+:     :+:+:   :+: :+:        :+:        ",
    "  +:+    +:++:+        :+:+:+  +:+     +:+         +:+     :+:+:+  +:+ +:+        +:+        ",
    "  +#++:++#+ +#++:++#   +#+ +:+ +#+     +#+         +#+     +#+ +:+ +#+ +#++:++#   +#+        ",
    "  +#+       +#+        +#+  +#+#+#     +#+         +#+     +#+  +#+#+# +#+        +#+        ",
    "  #+#       #+#        #+#   #+#+#     #+#         #+#     #+#   #+#+# #+#        #+#        ",
    " ###       ########## ###    #### ########### ########### ###    #### ########## ##########",
};
#define LOGO_LINES 7
#define LOGO_START_ROW 2

static const char* TAGLINE   = "[ SECURITY-FIRST MICROKERNEL  //  RING 0  //  x86-32 ]";
static const char* VERSION   = "v 0.1.0  -  sentinel.os";
static const char* BOOT_LABEL= "INITIALIZING KERNEL SUBSYSTEMS";

/* ── Utility: centered x position ──────────────────────────────────────── */
static int center_x(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return (VGA_WIDTH - len) / 2;
}

/* ── Busy-wait delay (calibrated for ~100 MHz emulated CPU) ─────────────  */
static void splash_delay(uint32_t count) {
    for (volatile uint32_t i = 0; i < count; i++)
        asm volatile("nop");
}

/* ── Draw the full loading bar ──────────────────────────────────────────── */
#define BAR_X    2
#define BAR_Y   21
#define BAR_W   76

static void draw_bar(int filled) {
    for (int i = 0; i < BAR_W; i++) {
        if (i < filled)
            vga_putchar(BAR_X + i, BAR_Y, '\xDB', STYLE_BAR_FILL);  /* █ */
        else
            vga_putchar(BAR_X + i, BAR_Y, '\xB0', STYLE_BAR_EMPTY); /* ░ */
    }
}

/* ── Public entry point ─────────────────────────────────────────────────── */
void splash_show(void) {
    /* 1. Wipe screen to pure black */
    vga_clear(VGA_ATTR(VGA_COLOR_BLACK, VGA_COLOR_BLACK));

    /* 2. Print the logo — typewriter style, line by line */
    for (int line = 0; line < LOGO_LINES; line++) {
        const char* row = LOGO[line];
        int col = 0;
        while (row[col]) {
            vga_putchar(col, LOGO_START_ROW + line, row[col], STYLE_LOGO);
            col++;
            splash_delay(2000);
        }
    }

    splash_delay(400000);

    /* 3. Tagline — fade in all at once */
    vga_puts(center_x(TAGLINE), 10, TAGLINE, STYLE_TAGLINE);
    splash_delay(300000);

    /* 4. Version string */
    vga_puts(center_x(VERSION), 12, VERSION, STYLE_LABEL);
    splash_delay(200000);

    /* 5. "INITIALIZING" label */
    vga_puts(center_x(BOOT_LABEL), 20, BOOT_LABEL, STYLE_LABEL);
    splash_delay(100000);

    /* 6. Animated loading bar — fills in chunks */
    draw_bar(0);
    for (int filled = 0; filled <= BAR_W; filled += 2) {
        draw_bar(filled);
        splash_delay(80000);
    }
    draw_bar(BAR_W);

    splash_delay(800000);

    /* 7. Clear — hand off to dashboard */
    vga_clear(VGA_ATTR(VGA_COLOR_BLACK, VGA_COLOR_BLACK));
}
