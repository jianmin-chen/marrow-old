#ifndef TAB_H
#define TAB_H

#include "../buffer/buffer.h"
#include <string.h>
#include <time.h>

extern struct row {
    int idx;
    int size;
    int rsize;
    char *chars;
    char *render;
} row;

extern struct tab {
    char *filename;
    char *swp;
    int numrows;
    struct row *rows;
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

void tabScroll(struct tab *t);

void tabUpdateRow(struct row *r);

void tabInsertRow(struct tab t, int at, char *s, size_t len);

void tabFreeRow(struct row *r);

struct tab tabOpen(char *filename, int screenrows, int screencols);

void tabSetStatusMessage(struct tab *t, const char *fmt, ...);

char *tabPrompt(struct tab *t, char *prompt, void (*render)(void),
                void (*callback)(char *, int));

void drawTab(struct tab *t, struct abuf *ab);

int tabNormalMode(struct tab *t, int key, void (*render)(void));

int tabEditMode(struct tab *t, int key, void (*render)(void));

#endif

