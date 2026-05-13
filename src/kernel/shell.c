#include "shell.h"
#include "tty.h"
#include "vga.h"
#include "keyboard.h"
#include "scheduler.h"
#include "memory.h"
#include "ipc.h"
#include "timer.h"
#include "vfs.h"
#include "fetch.h"
#include "matrix.h"

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
    tty_puts("\n");
    for(int i=0;i<41;i++) { tty_putchar('\xC4'); } tty_putchar('\n');
    tty_puts("  help    : Show this help message\n");
    tty_puts("  clear   : Clear the terminal\n");
    tty_puts("  mem     : Show kernel heap usage\n");
    tty_puts("  threads : Show active threads\n");
    tty_puts("  uptime  : Show system uptime\n");
    tty_puts("  about   : About SentinelOS\n");
    tty_puts("  reboot  : Restart the system\n");
    for(int i=0;i<41;i++) { tty_putchar('\xC4'); } tty_putchar('\n');
    tty_puts("  ls      : List files in RamFS\n");
    tty_puts("  cat     : Read file contents (e.g. cat readme.txt)\n");
    tty_puts("  fetch   : Show aesthetic system info\n");
    tty_puts("  matrix  : Digital rain screensaver\n");
    for(int i=0;i<41;i++) { tty_putchar('\xC4'); } tty_putchar('\n');
    tty_putchar('\n');
}

static void cmd_clear(void) {
    tty_clear();
}

static void cmd_mem(void) {
    uint32_t used = kheap_used();
    uint32_t total = 10 * 4096;
    uint32_t pct = (used * 100) / total;
    tty_puts("\n  Memory Report\n");
    tty_puts("  "); for(int i=0;i<41;i++) tty_putchar('\xC4'); tty_putchar('\n');
    tty_puts("  Heap used  : "); tty_put_uint(used);  tty_puts(" bytes\n");
    tty_puts("  Heap total : "); tty_put_uint(total); tty_puts(" bytes\n");
    tty_puts("  Usage      : "); tty_put_uint(pct);   tty_puts("%\n");
    tty_puts("  Phys pages : 128 MB mapped\n");
    tty_puts("  Paging     : Identity map (0x0 - 0x1000000)\n\n");
}

static void cmd_threads(void) {
    extern thread_t threads[];
    tty_puts("\n  Active Threads\n");
    tty_puts("  "); for(int i=0;i<41;i++) tty_putchar('\xC4'); tty_putchar('\n');
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
    /* CP437: \xDA=\u250C \xBF=\u2510 \xC0=\u2514 \xD9=\u2518 \xC4=\u2500 \xB3=\u2502 */
    tty_puts("  \xDA"); for(int i=0;i<43;i++) tty_putchar('\xC4'); tty_puts("\xBF\n");
    tty_puts("  \xB3         S E N T I N E L  O S          \xB3\n");
    tty_puts("  \xB3                                          \xB3\n");
    tty_puts("  \xB3  Version  : 0.1.0                        \xB3\n");
    tty_puts("  \xB3  Arch     : x86-32 Protected Mode        \xB3\n");
    tty_puts("  \xB3  Kernel   : Microkernel (Ring 0)         \xB3\n");
    tty_puts("  \xB3  Sched    : Preemptive Round-Robin        \xB3\n");
    tty_puts("  \xB3  Memory   : Bitmap PMM + Bump Heap        \xB3\n");
    tty_puts("  \xB3  Paging   : x86 Identity Map (16 MB)     \xB3\n");
    tty_puts("  \xB3  IPC      : Async Mailbox                 \xB3\n");
    tty_puts("  \xC0"); for(int i=0;i<43;i++) tty_putchar('\xC4'); tty_puts("\xD9\n\n");
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

/* ── Command dispatcher ──────────────────────────────────────────────────── */
static void dispatch(void) {
    /* Null-terminate */
    cmd_buf[cmd_len] = '\0';

    /* Echo the command to TTY */
    tty_puts("\n  > ");
    tty_puts(cmd_buf);
    tty_putchar('\n');

    if (cmd_len == 0)    {} else {
        /* Not empty, check if it matches a built-in */
        if (str_eq(cmd_buf, "help")) { cmd_help(); }
        else if (str_eq(cmd_buf, "clear")) { tty_clear(); }
        else if (str_eq(cmd_buf, "mem")) { cmd_mem(); }
        else if (str_eq(cmd_buf, "threads")) { cmd_threads(); }
        else if (str_eq(cmd_buf, "uptime")) { cmd_uptime(); }
        else if (str_eq(cmd_buf, "about")) { cmd_about(); }
        else if (str_eq(cmd_buf, "reboot")) { cmd_reboot(); }
        else if (str_eq(cmd_buf, "fetch")) { cmd_fetch(); }
        else if (str_eq(cmd_buf, "matrix")) { cmd_matrix(); }
        else if (str_eq(cmd_buf, "ls")) {
            tty_puts("\n");
            int i = 0;
            vfs_node_t* n = 0;
            while ((n = vfs_readdir(fs_root, i++)) != 0) {
                tty_puts("  "); tty_puts(n->name);
                tty_puts("  ("); tty_put_uint(n->length); tty_puts(" bytes)\n");
            }
        }
        else {
            /* Try `cat filename` */
            char* file_name = 0;
            for (int i=0; cmd_buf[i]; i++) {
                if (cmd_buf[i] == ' ' && cmd_buf[i+1] != 0) {
                    cmd_buf[i] = 0;
                    file_name = &cmd_buf[i+1];
                    break;
                }
            }
            if (str_eq(cmd_buf, "cat") && file_name) {
                vfs_node_t* n = vfs_finddir(fs_root, file_name);
                if (n) {
                    tty_puts("\n");
                    uint8_t buf[256];
                    uint32_t sz = vfs_read(n, 0, 255, buf);
                    buf[sz] = 0;
                    tty_puts((char*)buf);
                } else {
                    tty_puts("\nError: File not found.\n");
                }
            } else {
                tty_puts("\nUnknown command: '");
                tty_puts(cmd_buf);
                tty_puts("'\n");
            }
        }
    }

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
