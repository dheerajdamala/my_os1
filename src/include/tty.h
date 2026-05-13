#ifndef TTY_H
#define TTY_H

#include <stdint.h>

void tty_init(void);
void tty_putchar(char c);
void tty_puts(const char* s);
void tty_put_uint(uint32_t v);
void tty_put_hex(uint32_t v);
void tty_clear(void);
void tty_set_color(uint8_t fg, uint8_t bg);
void tty_reset_color(void);


/* Row/col bounds exported for shell input line */
#define TTY_TOP_ROW    1
#define TTY_BOT_ROW    18   /* inclusive, 18 lines of output */
#define TTY_LEFT_COL   1
#define TTY_RIGHT_COL  78   /* inclusive */
#define TTY_COLS       (TTY_RIGHT_COL - TTY_LEFT_COL + 1)  /* 78 */
#define TTY_ROWS       (TTY_BOT_ROW   - TTY_TOP_ROW  + 1)  /* 18 */

#define INPUT_ROW      19
#define INPUT_COL      1

#endif /* TTY_H */
