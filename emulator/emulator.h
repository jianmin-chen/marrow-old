#ifndef EMULATOR_H
#define EMULATOR_H

#include "../status/status.h"

#define SHELL "/bin/marrow"

typedef struct pty {
    int master;
    int slave;
} pty;

typedef struct row {
    int idx;
    int size;
    char *chars;
} row;

typedef struct emulator {
    pty *p;
    int numrows;
    row *rows;
    int cx, cy;
    int rowoff;
    int screenrows;
    int screencols;
    status *bar;
} emulator;

void spawn(pty *p);

emulator createEmulator(int screenrows, int screencols);

void emulatorScroll(emulator *e);

void drawEmulatorCursor(emulator *e, abuf *ab);

void drawEmulatorLine(emulator *e, abuf *ab, int y);

void emulatorReadKey(emulator *e, int key);

#endif
