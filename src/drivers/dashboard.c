#include "dashboard.h"
#include "vga.h"
#include "scheduler.h"
#include "memory.h"
#include "ipc.h"
#include "timer.h"

/*
 * SentinelOS Live Dashboard
 * ─────────────────────────
 * Persistent HUD rendered in VGA text mode (80×25).
 * The static chrome is drawn once by dashboard_init().
 * dashboard_update() only rewrites the live data cells — no flicker.
 *
 * Screen layout:
 *
 *  Row  0   ╔══ border ══╗
 *  Rows 1-7  ASCII logo (7 lines, bright cyan)
 *  Row  8    blank interior
 *  Row  9    tagline (magenta, centered)
 *  Row 10    blank interior
 *  Row 11   ╠══ separator ══╣
 *  Row 12    column header labels (dark grey)
 *  Row 13    live value row (white / cyan)
 *  Row 14    blank interior
 *  Row 15   ╠══ separator ══╣
 *  Row 16    KTF label row
 *  Row 17    KTF value row
 *  Row 18    blank interior
 *  Row 19   ╠══ separator ══╣
 *  Rows 20-22  memory bar
 *  Row 23    blank interior
 *  Row 24   ╚══ border ══╝
 */

/* ── Static strings ──────────────────────────────────────────────────────── */
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
#define LOGO_ROW   1

static const char* TAGLINE = "[ SECURITY-FIRST MICROKERNEL  //  RING 0  //  x86-32 ]";

/* Column header positions (row 12) */
#define COL_THREADS_X   2
#define COL_SWITCHES_X  16
#define COL_MEM_X       34
#define COL_IPC_X       52
#define COL_UPTIME_X    66
#define HDR_ROW         12
#define VAL_ROW         13

/* KTF stats positions (row 17) */
#define KTF_SEND_LBL_X  4
#define KTF_RECV_LBL_X  22
#define KTF_SWITCH_LBL_X 40
#define KTF_ROW_LBL     16
#define KTF_ROW_VAL     17

/* Memory bar (rows 20-22) */
#define MEM_BAR_X       2
#define MEM_BAR_Y       21
#define MEM_BAR_W       76

/* ── Internal state tracking for partial updates ─────────────────────────── */
static uint32_t last_threads  = 0xFFFFFFFF;
static uint32_t last_switches = 0xFFFFFFFF;
static uint32_t last_mem      = 0xFFFFFFFF;
static uint32_t last_ipc_sent = 0xFFFFFFFF;
static uint32_t last_ipc_recv = 0xFFFFFFFF;
static uint32_t last_uptime   = 0xFFFFFFFF;
static uint32_t last_mem_pct  = 0xFFFFFFFF;

/* ── Separators (full-width interior, between box walls) ─────────────────── */
static void draw_separator(int row) {
    vga_putchar(0,  row, '\xCC', STYLE_BORDER); /* ╠ */
    vga_putchar(79, row, '\xB9', STYLE_BORDER); /* ╣ */
    for (int x = 1; x < 79; x++)
        vga_putchar(x, row, '\xCD', STYLE_BORDER); /* ═ */
}

/* ── Static chrome (called once) ─────────────────────────────────────────── */
void dashboard_init(void) {
    vga_clear(VGA_ATTR(VGA_COLOR_BLACK, VGA_COLOR_BLACK));

    /* Outer box */
    vga_draw_box(0, 0, 80, 25, STYLE_BORDER);

    /* Logo */
    for (int i = 0; i < LOGO_LINES; i++)
        vga_puts(0, LOGO_ROW + i, LOGO[i], STYLE_LOGO);

    /* Tagline */
    {
        int len = 0; while (TAGLINE[len]) len++;
        vga_puts((80 - len) / 2, 9, TAGLINE, STYLE_TAGLINE);
    }

    /* Separator row 11 */
    draw_separator(11);

    /* Column headers row 12 */
    vga_puts(COL_THREADS_X,  HDR_ROW, "THREADS",   STYLE_LABEL);
    vga_puts(COL_SWITCHES_X, HDR_ROW, "CTX SWITCHES", STYLE_LABEL);
    vga_puts(COL_MEM_X,      HDR_ROW, "HEAP USED",  STYLE_LABEL);
    vga_puts(COL_IPC_X,      HDR_ROW, "IPC TX/RX",  STYLE_LABEL);
    vga_puts(COL_UPTIME_X,   HDR_ROW, "UPTIME",     STYLE_LABEL);

    /* Separator row 15 */
    draw_separator(15);

    /* KTF labels row 16 */
    vga_puts(KTF_SEND_LBL_X,   KTF_ROW_LBL, "IPC SENT",    STYLE_LABEL);
    vga_puts(KTF_RECV_LBL_X,   KTF_ROW_LBL, "IPC RECV",    STYLE_LABEL);
    vga_puts(KTF_SWITCH_LBL_X, KTF_ROW_LBL, "SCHED SWITCHES", STYLE_LABEL);

    /* Separator row 19 */
    draw_separator(19);

    /* Memory bar label row 20 */
    vga_puts(MEM_BAR_X, 20, "MEMORY USAGE", STYLE_LABEL);

    /* Initial bar fill (all empty) */
    for (int i = 0; i < MEM_BAR_W; i++)
        vga_putchar(MEM_BAR_X + i, MEM_BAR_Y, '\xB0', STYLE_BAR_EMPTY); /* ░ */

    /* Force first full repaint */
    last_threads = last_switches = last_mem = last_ipc_sent =
        last_ipc_recv = last_uptime = last_mem_pct = 0xFFFFFFFF;
}

/* ── Live update (called every timer tick from timer.c) ──────────────────── */
void dashboard_update(void) {
    uint32_t threads  = (uint32_t)get_thread_count();
    uint32_t switches = get_scheduler_switch_count();
    uint32_t mem_used = kheap_used();
    uint32_t ipc_sent, ipc_recv;
    ipc_get_stats(&ipc_sent, &ipc_recv);
    uint32_t uptime_s = timer_get_ticks() / 100; /* 100 Hz timer */

    /* Memory % (heap is 10 pages = 40 KB) */
    uint32_t heap_total = 10 * 4096;
    uint32_t mem_pct    = (mem_used * 100) / heap_total;

    /* ── Thread count ── */
    if (threads != last_threads) {
        vga_fill_rect(COL_THREADS_X, VAL_ROW, 10, 1, ' ', STYLE_DEFAULT);
        vga_put_uint(COL_THREADS_X, VAL_ROW, threads, STYLE_VALUE);
        last_threads = threads;
    }

    /* ── Context switches ── */
    if (switches != last_switches) {
        vga_fill_rect(COL_SWITCHES_X, VAL_ROW, 14, 1, ' ', STYLE_DEFAULT);
        vga_put_uint(COL_SWITCHES_X, VAL_ROW, switches, STYLE_VALUE);
        last_switches = switches;
    }

    /* ── Memory used (bytes) ── */
    if (mem_used != last_mem) {
        vga_fill_rect(COL_MEM_X, VAL_ROW, 14, 1, ' ', STYLE_DEFAULT);
        vga_put_uint(COL_MEM_X, VAL_ROW, mem_used, STYLE_VALUE);
        vga_puts(COL_MEM_X + 7, VAL_ROW, " B", STYLE_LABEL);
        last_mem = mem_used;
    }

    /* ── IPC TX / RX ── */
    if (ipc_sent != last_ipc_sent || ipc_recv != last_ipc_recv) {
        vga_fill_rect(COL_IPC_X, VAL_ROW, 18, 1, ' ', STYLE_DEFAULT);
        vga_put_uint(COL_IPC_X, VAL_ROW, ipc_sent, STYLE_VALUE);
        vga_puts(COL_IPC_X + 4, VAL_ROW, "/", STYLE_LABEL);
        vga_put_uint(COL_IPC_X + 5, VAL_ROW, ipc_recv, STYLE_VALUE);
        last_ipc_sent = ipc_sent;
        last_ipc_recv = ipc_recv;
    }

    /* ── Uptime (HH:MM:SS) ── */
    if (uptime_s != last_uptime) {
        uint32_t hh = uptime_s / 3600;
        uint32_t mm = (uptime_s % 3600) / 60;
        uint32_t ss = uptime_s % 60;
        vga_fill_rect(COL_UPTIME_X, VAL_ROW, 12, 1, ' ', STYLE_DEFAULT);
        vga_put_uint_padded(COL_UPTIME_X,     VAL_ROW, hh, 2, STYLE_VALUE);
        vga_puts           (COL_UPTIME_X + 2, VAL_ROW, ":", STYLE_LABEL);
        vga_put_uint_padded(COL_UPTIME_X + 3, VAL_ROW, mm, 2, STYLE_VALUE);
        vga_puts           (COL_UPTIME_X + 5, VAL_ROW, ":", STYLE_LABEL);
        vga_put_uint_padded(COL_UPTIME_X + 6, VAL_ROW, ss, 2, STYLE_VALUE);
        last_uptime = uptime_s;
    }

    /* ── KTF detail row ── */
    if (ipc_sent != last_ipc_sent || ipc_recv != last_ipc_recv ||
        switches  != last_switches) {
        vga_fill_rect(KTF_SEND_LBL_X,   KTF_ROW_VAL, 14, 1, ' ', STYLE_DEFAULT);
        vga_fill_rect(KTF_RECV_LBL_X,   KTF_ROW_VAL, 14, 1, ' ', STYLE_DEFAULT);
        vga_fill_rect(KTF_SWITCH_LBL_X, KTF_ROW_VAL, 18, 1, ' ', STYLE_DEFAULT);
        vga_put_uint(KTF_SEND_LBL_X,   KTF_ROW_VAL, ipc_sent,  STYLE_VALUE);
        vga_put_uint(KTF_RECV_LBL_X,   KTF_ROW_VAL, ipc_recv,  STYLE_VALUE);
        vga_put_uint(KTF_SWITCH_LBL_X, KTF_ROW_VAL, switches,  STYLE_VALUE);
    }

    /* ── Memory bar ── */
    if (mem_pct != last_mem_pct) {
        uint32_t filled = (mem_pct * MEM_BAR_W) / 100;
        for (int i = 0; i < MEM_BAR_W; i++) {
            if ((uint32_t)i < filled)
                vga_putchar(MEM_BAR_X + i, MEM_BAR_Y, '\xDB', STYLE_BAR_FILL);  /* █ */
            else
                vga_putchar(MEM_BAR_X + i, MEM_BAR_Y, '\xB0', STYLE_BAR_EMPTY); /* ░ */
        }
        /* Percentage label in bar */
        vga_fill_rect(35, MEM_BAR_Y, 10, 1, ' ', STYLE_BAR_FILL);
        vga_put_uint(37, MEM_BAR_Y, mem_pct, STYLE_VALUE);
        vga_puts(40,    MEM_BAR_Y, "%", STYLE_VALUE);
        last_mem_pct = mem_pct;
    }
}
