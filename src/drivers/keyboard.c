#include "keyboard.h"
#include "idt.h"
#include "io.h"

/* ── PS/2 Scancode Set 1 → ASCII (US QWERTY) ────────────────────────────
 * Index = scancode byte. 0 = non-printable / not mapped.
 * Two tables: unshifted and shifted.                                       */

static const char sc_ascii_low[128] = {
    0,   0,  '1','2','3','4','5','6','7','8','9','0','-','=', 0,  '\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0, '\\',
    'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  '-', 0,  0,  0,  '+', 0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

static const char sc_ascii_high[128] = {
    0,   0,  '!','@','#','$','%','^','&','*','(',')',  '_','+', 0, '\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}', '\n', 0,
    'A','S','D','F','G','H','J','K','L',':','"','~', 0, '|',
    'Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' ',
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  '-', 0,  0,  0,  '+', 0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

/* Backspace scancode */
#define SC_BACKSPACE 0x0E
/* Enter scancode */
#define SC_ENTER     0x1C
/* Left/Right Shift press/release */
#define SC_LSHIFT_DN 0x2A
#define SC_RSHIFT_DN 0x36
#define SC_LSHIFT_UP 0xAA
#define SC_RSHIFT_UP 0xB6

/* ── Ring buffer ─────────────────────────────────────────────────────────── */
#define KB_BUF_SIZE 256
static volatile char  kb_buf[KB_BUF_SIZE];
static volatile int   kb_head = 0;
static volatile int   kb_tail = 0;
static volatile int   kb_shift = 0;

static void kb_push(char c) {
    int next = (kb_tail + 1) % KB_BUF_SIZE;
    if (next != kb_head) {          /* drop if full */
        kb_buf[kb_tail] = c;
        kb_tail = next;
    }
}

/* ── IRQ1 handler ────────────────────────────────────────────────────────── */
static void keyboard_callback(registers_t* regs) {
    (void)regs;
    uint8_t sc = inb(0x60);        /* read scancode from PS/2 data port */

    /* Shift tracking */
    if (sc == SC_LSHIFT_DN || sc == SC_RSHIFT_DN) { kb_shift = 1; return; }
    if (sc == SC_LSHIFT_UP || sc == SC_RSHIFT_UP) { kb_shift = 0; return; }

    /* Key-release events have bit 7 set — ignore them */
    if (sc & 0x80) return;

    /* Backspace — push special sentinel '\b' */
    if (sc == SC_BACKSPACE) { kb_push('\b'); return; }

    /* Enter */
    if (sc == SC_ENTER) { kb_push('\n'); return; }

    /* Printable */
    if (sc < 128) {
        char c = kb_shift ? sc_ascii_high[sc] : sc_ascii_low[sc];
        if (c) kb_push(c);
    }
}

/* ── Public API ──────────────────────────────────────────────────────────── */
void keyboard_init(void) {
    register_interrupt_handler(33, keyboard_callback); /* IRQ1 = INT 33 */
}

char keyboard_getchar(void) {
    if (kb_head == kb_tail) return 0;  /* buffer empty */
    char c = kb_buf[kb_head];
    kb_head = (kb_head + 1) % KB_BUF_SIZE;
    return c;
}
