#include "serial.h"
#include "io.h"
#include <stdarg.h>

void serial_init(void) {
    outb(PORT_COM1 + 1, 0x00);
    outb(PORT_COM1 + 3, 0x80);
    outb(PORT_COM1 + 0, 0x03);
    outb(PORT_COM1 + 1, 0x00);
    outb(PORT_COM1 + 3, 0x03);
    outb(PORT_COM1 + 2, 0xC7);
    outb(PORT_COM1 + 4, 0x0B);
}

int is_transmit_empty(void) {
    return inb(PORT_COM1 + 5) & 0x20;
}

void serial_write_char(char c) {
    while (is_transmit_empty() == 0);
    outb(PORT_COM1, c);
}

void serial_write_string(const char* str) {
    while (*str) {
        serial_write_char(*str++);
    }
}

static void print_int(int val) {
    char buf[16];
    int i = 0;
    if (val == 0) { serial_write_char('0'); return; }
    if (val < 0) { serial_write_char('-'); val = -val; }
    while (val > 0) { buf[i++] = (val % 10) + '0'; val /= 10; }
    while (i > 0) serial_write_char(buf[--i]);
}

static void print_hex(uint32_t val) {
    char buf[16];
    int i = 0;
    if (val == 0) { serial_write_char('0'); return; }
    while (val > 0) {
        int rem = val % 16;
        if (rem < 10) buf[i++] = rem + '0'; else buf[i++] = rem - 10 + 'a';
        val /= 16;
    }
    serial_write_string("0x");
    while (i > 0) serial_write_char(buf[--i]);
}

void serial_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    while (*format != '\0') {
        if (*format == '%') {
            format++;
            if (*format == 'd') print_int(va_arg(args, int));
            else if (*format == 'x') print_hex(va_arg(args, uint32_t));
            else if (*format == 's') serial_write_string(va_arg(args, char*));
            else if (*format == 'c') serial_write_char((char)va_arg(args, int));
            else if (*format == '%') serial_write_char('%');
        } else {
            serial_write_char(*format);
        }
        format++;
    }
    va_end(args);
}
