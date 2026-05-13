#include "keyboard.h"
#include "io.h"
#include "idt.h"

/*
 * PS/2 Keyboard Driver — Poll-based (Milestone 1 fix)
 * ────────────────────────────────────────────────────
 * Instead of relying on IRQ1 firing (which QEMU may not deliver reliably
 * depending on machine type and display backend), we poll the PS/2 output
 * buffer status bit on every timer tick.
 *
 * PS/2 ports:
 *   0x60 = Data register   (read scancode here)
 *   0x64 = Status register (bit 0 = output buffer full = data ready)
 *
 * This is guaranteed to work in any QEMU configuration.
 */

/* ── Scancode Set 1 → ASCII (US QWERTY) ─────────────────────────────────── */
static const char sc_low[128] = {
    0,   0,  '1','2','3','4','5','6','7','8','9','0','-','=', 0, '\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0, '\\',
    'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,'-',0,0,0,'+',0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static const char sc_high[128] = {
    0,   0,  '!','@','#','$','%','^','&','*','(',')', '_','+', 0, '\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
    'A','S','D','F','G','H','J','K','L',':','"','~', 0, '|',
    'Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,'-',0,0,0,'+',0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

#define SC_BACKSPACE  0x0E
#define SC_ENTER      0x1C
#define SC_LSHIFT_DN  0x2A
#define SC_RSHIFT_DN  0x36
#define SC_LSHIFT_UP  0xAA
#define SC_RSHIFT_UP  0xB6

/* ── Ring buffer ─────────────────────────────────────────────────────────── */
#define KB_BUF_SIZE 256
static volatile char kb_buf[KB_BUF_SIZE];
static volatile int  kb_head = 0;
static volatile int  kb_tail = 0;
static volatile int  kb_shift = 0;

static void kb_push(char c) {
    int next = (kb_tail + 1) % KB_BUF_SIZE;
    if (next != kb_head) {
        kb_buf[kb_tail] = c;
        kb_tail = next;
    }
}

/* ── Process a single scancode byte ─────────────────────────────────────── */
static void kb_process(uint8_t sc) {
    if (sc == SC_LSHIFT_DN || sc == SC_RSHIFT_DN) { kb_shift = 1; return; }
    if (sc == SC_LSHIFT_UP || sc == SC_RSHIFT_UP) { kb_shift = 0; return; }
    if (sc & 0x80) return;           /* key release — ignore */
    if (sc == SC_BACKSPACE) { kb_push('\b'); return; }
    if (sc == SC_ENTER)     { kb_push('\n'); return; }
    if (sc < 128) {
        char c = kb_shift ? sc_high[sc] : sc_low[sc];
        if (c) kb_push(c);
    }
}

/* ── Polling function — called from timer IRQ every tick ─────────────────── */
void keyboard_poll(void) {
    /* Read all pending scancodes from the PS/2 output buffer */
    while (inb(0x64) & 0x01) {       /* bit 0 = output buffer full */
        uint8_t sc = inb(0x60);
        kb_process(sc);
    }
}

/* ── IRQ1 handler (kept as backup) ──────────────────────────────────────── */
static void keyboard_irq_handler(registers_t* regs) {
    (void)regs;
    /* Just drain — same as poll, avoids stale data in controller buffer */
    while (inb(0x64) & 0x01) {
        uint8_t sc = inb(0x60);
        kb_process(sc);
    }
}

/* ── Public API ──────────────────────────────────────────────────────────── */
void keyboard_init(void) {
    /* Ensure PS/2 keyboard interrupts are enabled in the controller */
    while (inb(0x64) & 0x02);         /* wait for input buffer empty */
    outb(0x64, 0x20);                 /* read controller config byte */
    while (!(inb(0x64) & 0x01));      /* wait for output buffer full */
    uint8_t cfg = inb(0x60);
    cfg |=  0x01;                     /* bit 0 = enable keyboard IRQ */
    cfg &= ~0x10;                     /* bit 4 = keyboard clock disable, clear it */
    while (inb(0x64) & 0x02);
    outb(0x64, 0x60);                 /* write controller config byte */
    while (inb(0x64) & 0x02);
    outb(0x60, cfg);

    /* Register IRQ1 as backup */
    register_interrupt_handler(33, keyboard_irq_handler);
}

char keyboard_getchar(void) {
    if (kb_head == kb_tail) return 0;
    char c = kb_buf[kb_head];
    kb_head = (kb_head + 1) % KB_BUF_SIZE;
    return c;
}
