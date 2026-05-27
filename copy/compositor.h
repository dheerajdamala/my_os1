#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include <stdint.h>

typedef struct window {
    int x, y;
    int w, h;
    int client_w, client_h;
    const char* title;
    int row_start;
    int row_count;
    int active;
} window_t;

void compositor_init(void);
void compositor_loop(void);

#endif // COMPOSITOR_H
