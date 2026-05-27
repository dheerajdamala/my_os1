#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

void keyboard_init(void);
void keyboard_poll(void);          /* call from timer tick to read PS/2 buffer */
char keyboard_getchar(void);       /* returns 0 if buffer empty */

#endif /* KEYBOARD_H */
