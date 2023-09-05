#ifndef TAB_H
#define TAB_H

#include "../highlight/highlight.h"
#include "../keyboard/keyboard.h"
#include "../libs/buffer.h"
#include "../status/status.h"
#include <string.h>
#include <time.h>

typedef struct row {
    int idx;
    int size;
    int rsize;
    char *chars;
    char *render;
    unsigned char *hl;
    int hl_open_comment;
    int changed;
} row;

typedef struct tab {
    char *filename;
    char *filetype;
    char *swp;
    int numrows;
    row *rows;
    status *bar;
    syntax *syn;
    keypress *keystrokes;
    int cx, cy;
    int rx;
    int gutter;
    int rowoff;
    int coloff;
    int screenrows;
    int screencols;
    int dirty;
} tab;

void tabScroll(tab *t);

void tabUpdateRow(tab *t, row *r);

void tabFreeRow(row *r);

tab tabOpen(char *filename, int screenrows, int screencols, status *s);

void drawTabCursor(tab *t, abuf *ab);

void drawTabLine(tab *t, abuf *ab, int y);

void drawTabBar(tab *t, abuf *ab);

int tabNormalMode(tab *t, int key, void (*render)(void));

int tabEditMode(tab *t, int key, void (*render)(void));

#endif
