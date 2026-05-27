#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

void mouse_init(void);

extern int mouse_x;
extern int mouse_y;
extern int mouse_left_click;
extern int mouse_right_click;
extern int mouse_updated;

#endif // MOUSE_H
