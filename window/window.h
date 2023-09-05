#ifndef WINDOW_H
#define WINDOW_H

#include <termios.h>

typedef struct window {
    struct termios terminal;
    int rows;
    int cols;
} window;

extern window win;

void disableRawMode(void);

void enableRawMode(void);

int getWindowSize(int *rows, int *cols);

void resize(int sig);

#endif
