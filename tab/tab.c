#include "../libs/buffer.h"
#include "../config.h"
#include "../keyboard/keyboard.h"
#include "../modes.h"
#include "../status/error.h"
#include "../status/status.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/*** defines ***/

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

/*** prototypes ***/

void tabUpdateRow(row *r);

/*** row operations ***/

int rowCxToRx(row *r, int cx) {
    int rx = 0;
    int j;
    for (j = 0; j < cx; j++) {
        if (r->chars[j] == '\t')
            rx += (MARROW_TAB_STOP - 1) - (rx % MARROW_TAB_STOP);
        rx++;
    }
    return rx;
}

int rowRxToCx(row *r, int rx) {
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

void tabRowInsertChar(row *r, int at, int c) {
    if (at < 0 || at > r->size)
        at = r->size;
    r->chars = realloc(r->chars, r->size + 2);
    memmove(&r->chars[at + 1], &r->chars[at], r->size - at * 1);
    r->size++;
    r->chars[at] = c;
    tabUpdateRow(r);
}

void tabRowDelChar(row *r, int at) {
    if (at < 0 || at >= r->size)
        return;
    memmove(&r->chars[at], &r->chars[at + 1], r->size - at);
    r->size--;
    tabUpdateRow(r);
}

void tabRowAppendString(row *r, char *s, size_t len) {
    r->chars = realloc(r->chars, r->size + len + 1);
    memcpy(&r->chars[r->size], s, len);
    r->size += len;
    r->chars[r->size] = '\0';
    tabUpdateRow(r);
}

/*** tab operations ***/

void tabUpdateRow(row *r) {
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

void tabInsertRow(tab *t, int at, char *s, size_t len) {
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

void tabFreeRow(row *r) {
    free(r->render);
    free(r->chars);
}

void tabDelRow(tab *t, int at) {
    if (at < 0 || at >= t->numrows)
        return;
    tabFreeRow(&t->rows[at]);
    memmove(&t->rows[at], &t->rows[at + 1],
            sizeof(row) * (t->numrows - at - 1));
    for (int j = at; j < t->numrows - 1; j++)
        t->rows[j].idx--;
    t->numrows--;
    t->dirty++;
}

void tabInsertNewline(tab *t) {
    if (t->cx == 0) {
        tabInsertRow(t, t->cy, "", 0);
    } else {
        row *r = &t->rows[t->cy];
        tabInsertRow(t, t->cy + 1, &r->chars[t->cx], r->size - t->cx);
        r = &t->rows[t->cy];
        r->size = t->cx;
        r->chars[r->size] = '\0';
        tabUpdateRow(r);
    }
    t->cy++;
    t->cx = 0;
}

void tabInsertChar(tab *t, int c) {
    if (t->cy == t->numrows) {
        tabInsertRow(t, t->numrows, "", 0);
    }
    tabRowInsertChar(&t->rows[t->cy], t->cx, c);
    t->cx++;
}

void tabDelChar(tab *t) {
    if (t->cy == t->numrows)
        return;

    row *r = &t->rows[t->cy];
    if (t->cx > 0) {
        tabRowDelChar(r, t->cx - 1);
        t->cx--;
    } else {
        t->cx = t->rows[t->cy - 1].size;
        tabRowAppendString(&t->rows[t->cy - 1], r->chars, r->size);
        tabDelRow(t, t->cy);
        t->cy--;
    }
}

/*** file operations ***/

tab tabOpen(char *filename, int screenrows, int screencols, status *s) {
    tab new;
    new.filename = strdup(filename);
    new.numrows = 0;
    new.rows = NULL;
    new.bar = s;
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

void tabSave(tab *t) {}

void tabScroll(tab *t) {
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

void drawTab(tab *t, abuf *ab) {
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

    // Draw status bar
    abAppend(ab, "\x1b[7m", 4);
    char status[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
                       t->filename ? t->filename : "[No Name]", t->numrows,
                       t->dirty ? "(modified)" : "");

    if (len > t->screencols)
        len = t->screencols;
    abAppend(ab, status, len);
    while (len < t->screencols) {
        abAppend(ab, " ", 1);
        len++;
    }

    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

/*** cursor operations ***/

void tabMoveCursor(tab *t, int key) {
    row *r = (t->cy >= t->numrows) ? NULL : &t->rows[t->cy];

    switch (key) {
    case ARROW_LEFT:
    case H:
        if (t->cx != 0) {
            t->cx--;
        } else if (t->cy > 0) {
            t->cy--;
            t->cx = t->rows[t->cy].size;
        }
        break;
    case ARROW_RIGHT:
    case L:
        if (r && t->cx < r->size) {
            t->cx++;
        } else if (r && t->cx == r->size) {
            t->cy++;
            t->cx = 0;
        }
        break;
    case ARROW_UP:
    case K:
        if (t->cy != 0) {
            t->cy--;
        }
        break;
    case ARROW_DOWN:
    case J:
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

void tabJumpTo(tab *t, char *buf, int key) {
    // Convert buf to int
    for (unsigned long i = 0; i < strlen(buf); i++) {
        if (!isdigit(buf[i]))
            return;
    }

    int line = atoi(buf);
    if (line == 0)
        line = 1;
    else if (line > t->numrows)
        line = t->numrows;
    t->cy = line - 1;
}

/*** prompt ***/

char *tabPrompt(tab *t, char *prompt, void (*render)(void),
                void (*callback)(tab *t, char *, int)) {
    // Prompt, specifically for tabs
    size_t bufsize = 128;
    char *buf = malloc(bufsize);

    size_t buflen = 0;
    buf[0] = '\0';

    status *s = t->bar;

    while (1) {
        snprintf(s->statusmsg, sizeof(s->statusmsg), "%s%s", prompt, buf);
        s->statusmsg_time = time(NULL);
        render();

        int c = editorReadKey();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buflen != 0)
                buf[--buflen] = '\0';
        } else if (c == '\x1b') {
            setStatusMessage(s, "");
            free(buf);
            return NULL;
        } else if (c == '\r') {
            if (buflen != 0) {
                setStatusMessage(s, "");
                if (callback)
                    callback(t, buf, c);
                return buf;
            }
        } else if (!iscntrl(c) && c < 128) {
            if (buflen == bufsize - 1) {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }
    }
}

/*** modes ***/

int tabNormalMode(tab *t, int key, void (*render)(void)) {
    switch (key) {
    case DOLLAR:
        t->cx = t->rows[t->cy].size;
        break;
    case ZERO:
        t->cx = 0;
        break;
    case HOME_KEY:
        t->cx = 0;
        break;
    case END_KEY:
        if (t->cy < t->numrows)
            t->cx = t->rows[t->cy].size;
        break;
    case PAGE_UP:
    case PAGE_DOWN: {
        if (key == PAGE_UP)
            t->cy = t->rowoff;
        else if (key == PAGE_DOWN) {
            t->cy = t->rowoff + t->screenrows - 1;
            if (t->cy > t->numrows)
                t->cy = t->numrows;
        }
    } break;
    case COLON:
        tabPrompt(t, ":", render, tabJumpTo);
        break;
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
    case H:
    case J:
    case K:
    case L:
        tabMoveCursor(t, key);
        break;
    case I:
        setStatusMessage(t->bar, "Switching to NORMAL");
        return EDIT;
    }

    return NORMAL;
}

int tabEditMode(tab *t, int key, void (*render)(void)) {
    switch (key) {
    case ESC:
        setStatusMessage(t->bar, "Switching to EDIT");
        return NORMAL;
    case CTRL_KEY('s'):
        break;
    case HOME_KEY:
        t->cx = 0;
        break;
    case END_KEY:
        if (t->cy < t->numrows)
            t->cx = t->rows[t->cy].size;
    case PAGE_UP:
    case PAGE_DOWN: {
        if (key == PAGE_UP) {
            t->cy = t->rowoff;
        } else if (key == PAGE_DOWN) {
            t->cy = t->rowoff + t->screenrows - 1;
            if (t->cy > t->numrows)
                t->cy = t->numrows;
        }
    } break;
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
        tabMoveCursor(t, key);
        break;
    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
        if (key == DEL_KEY)
            tabMoveCursor(t, ARROW_RIGHT);
        tabDelChar(t);
        t->dirty++;
        break;
    case '\r':
        tabInsertNewline(t);
        break;
    default:
        tabInsertChar(t, key);
        t->dirty++;
        break;
    }
    return EDIT;
}

