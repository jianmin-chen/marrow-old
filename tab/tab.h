#ifndef TAB_H
#define TAB_H

#include "../buffer/buffer.h"
#include <string.h>
#include <time.h>

typedef struct row {
    int idx;
    int size;
    int rsize;
    char *chars;
    char *render;
} row;

typedef struct tab {
    char *filename;
    char *swp;
    int numrows;
    row *rows;
    int cx, cy;
    int rx;
    int rowoff;
    int coloff;
    int screenrows;
    int screencols;
    int dirty;
    char statusmsg[80];
    time_t statusmsg_time;
} tab;

void tabScroll(tab *t);

void tabUpdateRow(row *r);

void tabInsertRow(tab t, int at, char *s, size_t len);

void tabFreeRow(row *r);

tab tabOpen(char *filename, int screenrows, int screencols);

void tabSetStatusMessage(tab *t, const char *fmt, ...);

char *tabPrompt(tab *t, char *prompt, void (*render)(void),
                void (*callback)(char *, int));

void drawTab(tab *t, abuf *ab);

int tabNormalMode(tab *t, int key, void (*render)(void));

int tabEditMode(tab *t, int key, void (*render)(void));

#endif

