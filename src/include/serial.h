#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

#define PORT_COM1 0x3f8

void serial_init(void);
void serial_write_char(char c);
void serial_write_string(const char* str);
void serial_printf(const char* format, ...);

#endif // SERIAL_H
