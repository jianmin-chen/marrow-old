#ifndef TAB_H
#define TAB_H

#include "../libs/buffer.h"
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
} row;

typedef struct tab {
    char *filename;
    char *filetype;
    char *swp;
    int numrows;
    row *rows;
    status *bar;
    int cx, cy;
    int rx;
    int rowoff;
    int coloff;
    int screenrows;
    int screencols;
    int dirty;
} tab;

void tabScroll(tab *t);

void tabUpdateRow(row *r);

void tabInsertRow(tab t, int at, char *s, size_t len);

void tabFreeRow(row *r);

tab tabOpen(char *filename, int screenrows, int screencols, status *s);

void drawTab(tab *t, abuf *ab, colors theme);

int tabNormalMode(tab *t, int key, void (*render)(void));

int tabEditMode(tab *t, int key, void (*render)(void));

#endif

