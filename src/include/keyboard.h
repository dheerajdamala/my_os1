#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

void keyboard_init(void);
char keyboard_getchar(void);   /* returns 0 if buffer empty */

#endif /* KEYBOARD_H */
