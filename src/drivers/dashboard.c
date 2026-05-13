#include "dashboard.h"
#include "vga.h"
#include "scheduler.h"
#include "memory.h"
#include "ipc.h"
#include "timer.h"

/*
 * SentinelOS Dashboard — Compact Bottom Bar (Milestone 1 redesign)
 * ─────────────────────────────────────────────────────────────────
 * New 80×25 layout:
 *
 *  Row  0   ╔══ SENTINEL OS  v0.1 ═══════════════════ HH:MM:SS ══╗
 *  Rows 1-18  TTY region (managed by tty.c)
 *  Row 19   ║ sentinel> █ (input line, managed by shell.c)        ║
 *  Row 20   ╠════════════════════════════════════════════════════╣
 *  Row 21   ║ THREADS: N  │  CTX SW: N  │  IPC: N/N  │  MEM: N% ║
 *  Row 22   ╠════════════════════════════════════════════════════╣
 *  Row 23   ║ mem bar ░░░░░░░░░░░░░░░░░░░░░░░░░  XX%            ║
 *  Row 24   ╚════════════════════════════════════════════════════╝
 *
 * dashboard_init() draws the static chrome (borders, labels, separators).
 * dashboard_update() rewrites only the live-value cells per timer tick.
 */

/* ── Positions ───────────────────────────────────────────────────────────── */
#define HEADER_ROW   0
#define SEP1_ROW    20
#define STATS_ROW   21
#define SEP2_ROW    22
#define BAR_ROW     23
#define FOOTER_ROW  24

#define BAR_X    1
#define BAR_W   78

/* ── Static chrome ───────────────────────────────────────────────────────── */

static void draw_separator(int row) {
    vga_putchar(0,  row, '\xCC', STYLE_BORDER); /* ╠ */
    vga_putchar(79, row, '\xB9', STYLE_BORDER); /* ╣ */
    for (int x = 1; x < 79; x++)
        vga_putchar(x, row, '\xCD', STYLE_BORDER);
}

static void draw_top_border(void) {
    vga_putchar(0,  HEADER_ROW, '\xC9', STYLE_BORDER); /* ╔ */
    vga_putchar(79, HEADER_ROW, '\xBB', STYLE_BORDER); /* ╗ */
    for (int x = 1; x < 79; x++)
        vga_putchar(x, HEADER_ROW, '\xCD', STYLE_BORDER);
    /* Title */
    vga_puts(2, HEADER_ROW, " SENTINEL OS  v0.1 ", STYLE_HEADER);
}

static void draw_bottom_border(void) {
    vga_putchar(0,  FOOTER_ROW, '\xC8', STYLE_BORDER); /* ╚ */
    vga_putchar(79, FOOTER_ROW, '\xBC', STYLE_BORDER); /* ╝ */
    for (int x = 1; x < 79; x++)
        vga_putchar(x, FOOTER_ROW, '\xCD', STYLE_BORDER);
    vga_puts(2, FOOTER_ROW, " SentinelOS  //  Ring 0  //  x86-32 ", STYLE_LABEL);
}

static void draw_side_borders(void) {
    for (int row = 1; row < FOOTER_ROW; row++) {
        vga_putchar(0,  row, '\xBA', STYLE_BORDER); /* ║ */
        vga_putchar(79, row, '\xBA', STYLE_BORDER);
    }
}

/* ── Last values for diff-update ─────────────────────────────────────────── */
static uint32_t last_threads  = 0xFFFFFFFF;
static uint32_t last_switches = 0xFFFFFFFF;
static uint32_t last_ipc_sent = 0xFFFFFFFF;
static uint32_t last_ipc_recv = 0xFFFFFFFF;
static uint32_t last_uptime   = 0xFFFFFFFF;
static uint32_t last_mem_pct  = 0xFFFFFFFF;

/* ── Public API ──────────────────────────────────────────────────────────── */

void dashboard_init(void) {
    vga_clear(VGA_ATTR(VGA_COLOR_BLACK, VGA_COLOR_BLACK));

    draw_top_border();
    draw_side_borders();
    draw_separator(SEP1_ROW);
    draw_separator(SEP2_ROW);
    draw_bottom_border();

    /* Static stat labels on row 21 */
    vga_puts( 2, STATS_ROW, "THREADS:", STYLE_LABEL);
    vga_puts(22, STATS_ROW, "CTX SW:",  STYLE_LABEL);
    vga_puts(40, STATS_ROW, "IPC TX/RX:", STYLE_LABEL);
    vga_puts(62, STATS_ROW, "UPTIME:",  STYLE_LABEL);

    /* Memory bar label on row 23 */
    vga_puts(2, BAR_ROW, "MEM", STYLE_LABEL);
    /* Initial empty bar */
    for (int i = 0; i < BAR_W - 6; i++)
        vga_putchar(BAR_X + 5 + i, BAR_ROW, '\xB0', STYLE_BAR_EMPTY);

    /* Force first full update */
    last_threads = last_switches = last_ipc_sent =
        last_ipc_recv = last_uptime = last_mem_pct = 0xFFFFFFFF;
}

void dashboard_update(void) {
    uint32_t threads  = (uint32_t)get_thread_count();
    uint32_t switches = get_scheduler_switch_count();
    uint32_t mem_used = kheap_used();
    uint32_t ipc_sent, ipc_recv;
    ipc_get_stats(&ipc_sent, &ipc_recv);
    uint32_t ticks    = timer_get_ticks();
    uint32_t uptime_s = ticks / 100;

    uint32_t heap_total = 10 * 4096;
    uint32_t mem_pct    = (mem_used * 100) / heap_total;

    /* ── Clock in header (right side, updates every second) ── */
    if (uptime_s != last_uptime) {
        uint32_t hh = uptime_s / 3600;
        uint32_t mm = (uptime_s % 3600) / 60;
        uint32_t ss = uptime_s % 60;
        vga_fill_rect(67, HEADER_ROW, 11, 1, '\xCD', STYLE_BORDER);
        vga_putchar(67, HEADER_ROW, ' ', STYLE_HEADER);
        vga_put_uint_padded(68, HEADER_ROW, hh, 2, STYLE_HEADER);
        vga_putchar(70, HEADER_ROW, ':', STYLE_LABEL);
        vga_put_uint_padded(71, HEADER_ROW, mm, 2, STYLE_HEADER);
        vga_putchar(73, HEADER_ROW, ':', STYLE_LABEL);
        vga_put_uint_padded(74, HEADER_ROW, ss, 2, STYLE_HEADER);
        vga_putchar(76, HEADER_ROW, ' ', STYLE_HEADER);
        last_uptime = uptime_s;
    }

    /* ── Thread count ── */
    if (threads != last_threads) {
        vga_fill_rect(11, STATS_ROW, 7, 1, ' ', STYLE_DEFAULT);
        vga_put_uint(11, STATS_ROW, threads, STYLE_VALUE);
        last_threads = threads;
    }

    /* ── Context switches ── */
    if (switches != last_switches) {
        vga_fill_rect(30, STATS_ROW, 8, 1, ' ', STYLE_DEFAULT);
        vga_put_uint(30, STATS_ROW, switches, STYLE_VALUE);
        last_switches = switches;
    }

    /* ── IPC TX/RX ── */
    if (ipc_sent != last_ipc_sent || ipc_recv != last_ipc_recv) {
        vga_fill_rect(51, STATS_ROW, 9, 1, ' ', STYLE_DEFAULT);
        vga_put_uint(51, STATS_ROW, ipc_sent, STYLE_VALUE);
        vga_putchar(55, STATS_ROW, '/', STYLE_LABEL);
        vga_put_uint(56, STATS_ROW, ipc_recv, STYLE_VALUE);
        last_ipc_sent = ipc_sent;
        last_ipc_recv = ipc_recv;
    }

    /* ── Uptime (HH:MM:SS in stats row) ── */
    if (uptime_s != last_uptime) { /* already updated above */ }

    /* ── Memory bar ── */
    if (mem_pct != last_mem_pct) {
        int bar_inner_w = BAR_W - 6;
        int filled = (int)((mem_pct * bar_inner_w) / 100);
        for (int i = 0; i < bar_inner_w; i++) {
            if (i < filled)
                vga_putchar(BAR_X + 5 + i, BAR_ROW, '\xDB', STYLE_BAR_FILL);
            else
                vga_putchar(BAR_X + 5 + i, BAR_ROW, '\xB0', STYLE_BAR_EMPTY);
        }
        /* Percentage label */
        vga_fill_rect(68, BAR_ROW, 10, 1, ' ', STYLE_DEFAULT);
        vga_put_uint(68, BAR_ROW, mem_pct, STYLE_VALUE);
        vga_puts(71, BAR_ROW, "%", STYLE_VALUE);
        last_mem_pct = mem_pct;
    }
}
