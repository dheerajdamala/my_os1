#include "shell.h"
#include "tty.h"
#include "vga.h"
#include "keyboard.h"
#include "scheduler.h"
#include "memory.h"
#include "ipc.h"
#include "timer.h"

#define PROMPT      "sentinel> "
#define PROMPT_LEN  10
#define MAX_CMD     68

static char cmd_buf[MAX_CMD + 1];
static int  cmd_len = 0;

/* ── String helpers (no libc) ────────────────────────────────────────────── */
static int str_eq(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == '\0' && *b == '\0';
}


/* ── Input line rendering ────────────────────────────────────────────────── */

/* Clear the input row and re-draw prompt + current buffer */
static void input_redraw(void) {
    /* Clear entire input row between box walls */
    vga_fill_rect(INPUT_COL, INPUT_ROW, 78, 1, ' ', STYLE_DEFAULT);
    /* Prompt in magenta */
    vga_puts(INPUT_COL, INPUT_ROW, PROMPT, STYLE_ACCENT);
    /* Command text in white */
    for (int i = 0; i < cmd_len; i++)
        vga_putchar(INPUT_COL + PROMPT_LEN + i, INPUT_ROW, cmd_buf[i], STYLE_VALUE);
    /* Cursor block */
    vga_putchar(INPUT_COL + PROMPT_LEN + cmd_len, INPUT_ROW, ' ',
                VGA_ATTR(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_CYAN));
}

/* ── Built-in commands ───────────────────────────────────────────────────── */

static void cmd_help(void) {
    tty_puts("\n  Available commands:\n");
    tty_puts("  ─────────────────────────────────────────\n");
    tty_puts("  help     Show this help message\n");
    tty_puts("  clear    Clear the terminal\n");
    tty_puts("  mem      Memory usage details\n");
    tty_puts("  threads  List active kernel threads\n");
    tty_puts("  uptime   Show system uptime\n");
    tty_puts("  about    About SentinelOS\n");
    tty_puts("  reboot   Reboot the system\n");
    tty_puts("  ─────────────────────────────────────────\n\n");
}

static void cmd_clear(void) {
    tty_clear();
}

static void cmd_mem(void) {
    uint32_t used = kheap_used();
    uint32_t total = 10 * 4096;  /* 10 pages heap */
    uint32_t pct = (used * 100) / total;
    tty_puts("\n  Memory Report\n");
    tty_puts("  ─────────────────────────────────────────\n");
    tty_puts("  Heap used  : "); tty_put_uint(used);  tty_puts(" bytes\n");
    tty_puts("  Heap total : "); tty_put_uint(total); tty_puts(" bytes\n");
    tty_puts("  Usage      : "); tty_put_uint(pct);   tty_puts("%\n");
    tty_puts("  Phys pages : 128 MB mapped\n");
    tty_puts("  Paging     : Identity map (0x0 - 0x400000)\n\n");
}

static void cmd_threads(void) {
    extern thread_t threads[];
    tty_puts("\n  Active Threads\n");
    tty_puts("  ─────────────────────────────────────────\n");
    tty_puts("  ID   STATE\n");
    int found = 0;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].state != THREAD_FREE) {
            tty_puts("  ");
            tty_put_uint(threads[i].id);
            tty_puts("    ");
            switch (threads[i].state) {
                case THREAD_READY:   tty_puts("READY\n");   break;
                case THREAD_RUNNING: tty_puts("RUNNING\n"); break;
                case THREAD_BLOCKED: tty_puts("BLOCKED\n"); break;
                default:             tty_puts("UNKNOWN\n"); break;
            }
            found++;
        }
    }
    if (!found) tty_puts("  (none)\n");
    tty_puts("\n");
}

static void cmd_uptime(void) {
    uint32_t ticks = timer_get_ticks();
    uint32_t s  = ticks / 100;
    uint32_t ms = (ticks % 100) * 10;
    uint32_t hh = s / 3600;
    uint32_t mm = (s % 3600) / 60;
    uint32_t ss = s % 60;
    tty_puts("\n  Uptime: ");
    if (hh < 10) tty_puts("0");
    tty_put_uint(hh); tty_puts(":");
    if (mm < 10) tty_puts("0");
    tty_put_uint(mm); tty_puts(":");
    if (ss < 10) tty_puts("0");
    tty_put_uint(ss); tty_puts(".");
    if (ms < 100) tty_puts("0");
    if (ms < 10)  tty_puts("0");
    tty_put_uint(ms);
    tty_puts("  ("); tty_put_uint(ticks); tty_puts(" ticks @ 100Hz)\n\n");
}

static void cmd_about(void) {
    tty_puts("\n");
    tty_puts("  ┌─────────────────────────────────────────┐\n");
    tty_puts("  │           S E N T I N E L  O S          │\n");
    tty_puts("  │                                          │\n");
    tty_puts("  │  Version  : 0.1.0                        │\n");
    tty_puts("  │  Arch     : x86-32 Protected Mode        │\n");
    tty_puts("  │  Kernel   : Microkernel (Ring 0)         │\n");
    tty_puts("  │  Sched    : Preemptive Round-Robin        │\n");
    tty_puts("  │  Memory   : Bitmap PMM + Bump Heap        │\n");
    tty_puts("  │  Paging   : x86 Identity Map (4 MB)      │\n");
    tty_puts("  │  IPC      : Async Mailbox                 │\n");
    tty_puts("  └─────────────────────────────────────────┘\n\n");
}

static void cmd_reboot(void) {
    tty_puts("\n  Rebooting...\n");
    for (volatile int i = 0; i < 5000000; i++);
    /* Triple-fault: load a null IDTR and trigger divide-by-zero */
    struct { uint16_t limit; uint32_t base; } __attribute__((packed)) null_idt = {0, 0};
    asm volatile("lidt %0" : : "m"(null_idt));
    asm volatile("int $0");
    while (1);
}


static void cmd_unknown(void) {
    tty_puts("  Unknown command: '");
    tty_puts(cmd_buf);
    tty_puts("' — type 'help'\n\n");
}

/* ── Command dispatcher ──────────────────────────────────────────────────── */
static void dispatch(void) {
    /* Null-terminate */
    cmd_buf[cmd_len] = '\0';

    /* Echo the command to TTY */
    tty_puts("\n  > ");
    tty_puts(cmd_buf);
    tty_putchar('\n');

    if (cmd_len == 0)           { /* empty — do nothing */ }
    else if (str_eq(cmd_buf, "help"))    cmd_help();
    else if (str_eq(cmd_buf, "clear"))   cmd_clear();
    else if (str_eq(cmd_buf, "mem"))     cmd_mem();
    else if (str_eq(cmd_buf, "threads")) cmd_threads();
    else if (str_eq(cmd_buf, "uptime"))  cmd_uptime();
    else if (str_eq(cmd_buf, "about"))   cmd_about();
    else if (str_eq(cmd_buf, "reboot"))  cmd_reboot();
    else                                 cmd_unknown();

    /* Reset buffer */
    cmd_len = 0;
}

/* ── Public API ──────────────────────────────────────────────────────────── */

void shell_init(void) {
    cmd_len = 0;
    input_redraw();
}

/* This is a kernel thread entry point — runs in an infinite loop */
void shell_thread(void) {
    shell_init();
    while (1) {
        char c = keyboard_getchar();
        if (c == 0) {
            scheduler_yield();
            continue;
        }

        if (c == '\n') {
            /* Execute command */
            dispatch();
            input_redraw();
        } else if (c == '\b') {
            /* Backspace */
            if (cmd_len > 0) cmd_len--;
            input_redraw();
        } else if (c >= ' ' && cmd_len < MAX_CMD) {
            cmd_buf[cmd_len++] = c;
            input_redraw();
        }
    }
}
