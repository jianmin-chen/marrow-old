#include "../buffer/buffer.h"
#include "../config.h"
#include "../error/error.h"
#include "../keyboard/keyboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

struct row {
    int idx;
    int size;
    int rsize;
    char *chars;
    char *render;
} row;

struct tab {
    char *filename;
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

/*** row operations ***/

int rowCxToRx(struct row *r, int cx) {
    int rx = 0;
    int j;
    for (j = 0; j < cx; j++) {
        if (r->chars[j] == '\t')
            rx += (MARROW_TAB_STOP - 1) - (rx % MARROW_TAB_STOP);
        rx++;
    }
    return rx;
}

int rowRxToCx(struct row *r, int rx) {
    int cur_rx = 0;
    int cx;
    for (cx = 0; cx < r->size; cx++) {
        if (r->chars[cx] == '\t')
            cur_rx += (MARROW_TAB_STOP - 1) - (cur_rx % MARROW_TAB_STOP);
        cur_rx++;
        if (cur_rx > rx)
            return cx;
    }
    return cx;
}

void tabUpdateRow(struct row *r) {
    int tabs = 0;
    int j;
    for (j = 0; j < r->size; j++)
        if (r->chars[j] == '\t')
            tabs++;

    free(r->render);
    r->render = malloc(r->size + tabs * (MARROW_TAB_STOP - 1) + 1);

    int idx = 0;
    for (j = 0; j < r->size; j++) {
        if (r->chars[j] == '\t') {
            r->render[idx++] = ' ';
            while (idx % MARROW_TAB_STOP != 0)
                r->render[idx++] = ' ';
        } else {
            r->render[idx++] = r->chars[j];
        }
    }
    r->render[idx] = '\0';
    r->rsize = idx;
}

void tabInsertRow(struct tab *t, int at, char *s, size_t len) {
    if (at < 0 || at > t->numrows)
        return;

    t->rows = realloc(t->rows, sizeof(row) * (t->numrows + 1));
    memmove(&t->rows[at + 1], &t->rows[at], sizeof(row) * (t->numrows - at));

    for (int j = at + 1; j <= t->numrows; j++)
        t->rows[j].idx++;

    t->rows[at].idx = at;

    t->rows[at].size = len;
    t->rows[at].chars = malloc(len + 1);
    memcpy(t->rows[at].chars, s, len);
    t->rows[at].chars[len] = '\0';

    t->rows[at].rsize = 0;
    t->rows[at].render = NULL;

    tabUpdateRow(&t->rows[at]);

    t->numrows++;
}

void tabFreeRow(struct row *r) {
    free(r->render);
    free(r->chars);
}

struct tab tabOpen(char *filename, int screenrows, int screencols) {
    struct tab new;
    new.filename = strdup(filename);
    new.numrows = 0;
    new.rows = NULL;
    new.cx = 0;
    new.cy = 0;
    new.rx = 0;
    new.rowoff = 0;
    new.coloff = 0;
    new.screenrows = screenrows;
    new.screencols = screencols;
    new.dirty = 0;

    if (access(filename, F_OK) != 0) {
        // Create file
        FILE *fptr = fopen(filename, "w");
        fclose(fptr);
    }

    FILE *fp = fopen(filename, "r");
    if (!fp)
        die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 &&
               (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            linelen--;
        tabInsertRow(&new, new.numrows, line, linelen);
    }

    free(line);
    fclose(fp);
    return new;
}

void tabScroll(struct tab *t) {
    t->rx = t->cx;
    if (t->cy < t->numrows)
        t->rx = rowCxToRx(&t->rows[t->cy], t->cx);

    if (t->cy < t->rowoff)
        t->rowoff = t->cy;
    if (t->cy >= t->rowoff + t->screenrows)
        t->rowoff = t->cy - t->screenrows + 1;
    if (t->rx < t->coloff)
        t->coloff = t->rx;
    if (t->rx >= t->coloff + t->screencols)
        t->coloff = t->rx - t->screencols + 1;
}

void drawTab(struct tab *t, struct abuf *ab) {
    tabScroll(t);

    int y;
    for (y = 0; y < t->screenrows; y++) {
        int filerow = y + t->rowoff;
        if (filerow >= t->numrows) {
            abAppend(ab, "~", 1);
        } else {
            int len = t->rows[filerow].rsize - t->coloff;
            if (len < 0)
                len = 0;
            if (len > t->screencols)
                len = t->screencols;
            char *c = &t->rows[filerow].render[t->coloff];
            int j;
            for (j = 0; j < len; j++) {
                abAppend(ab, &c[j], 1);
            }
        }
        abAppend(ab, "\x1b[K", 3);
        abAppend(ab, "\r\n", 2);
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (t->cy - t->rowoff) + 1,
             (t->rx - t->coloff) + 1);
    abAppend(ab, buf, strlen(buf));
}

void tabNormalMode(struct tab *t, int key) {
    struct row *r = (t->cy >= t->numrows) ? NULL : &t->rows[t->cy];

    switch (key) {
    case ARROW_LEFT:
        if (t->cx != 0) {
            t->cx--;
        } else if (t->cy > 0) {
            t->cy--;
            t->cx = t->rows[t->cy].size;
        }
        break;
    case ARROW_RIGHT:
        if (r && t->cx < r->size) {
            t->cx++;
        } else if (r && t->cx == r->size) {
            t->cy++;
            t->cx = 0;
        }
        break;
    case ARROW_UP:
        if (t->cy != 0) {
            t->cy--;
        }
        break;
    case ARROW_DOWN:
        if (t->cy < t->numrows) {
            t->cy++;
        }
        break;
    }

    r = (t->cy >= t->numrows) ? NULL : &t->rows[t->cy];
    int rowlen = r ? r->size : 0;
    if (t->cx > rowlen)
        t->cx = rowlen;
}

