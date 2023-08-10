#include "../config.h"
#include "../highlight/highlight.h"
#include "../keyboard/keyboard.h"
#include "../libs/buffer.h"
#include "../modes.h"
#include "../status/error.h"
#include "../status/status.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
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
    char *filetype;
    char *swp;
    int numrows;
    row *rows;
    status *bar;
    syntax *syn;
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
void dirty(tab *t);
void tabBackup(tab *t);
char *tabPrompt(tab *t, char *prompt, void (*render)(void),
                void (*callback)(tab *t, char *, int), int onchange);

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

/*** syntax related row operations ***/

void updateSyntax(tab *t, row *r) {
    r->hl = realloc(r->hl, r->rsize);
    memset(r->hl, HL_NORMAL, r->rsize);

    if (t->syn == NULL)
        return;
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

    int tabs = 0;
    if (at != 0) {
        // Remain on same tab length
        int j;
        for (j = 0; j < t->rows[at - 1].size; j++)
            if (t->rows[at - 1].chars[j] == '\t')
                tabs++;
    }

    t->rows[at].size = len + (tabs * MARROW_TAB_STOP);
    t->rows[at].chars = malloc(len + (tabs * MARROW_TAB_STOP) + 1);
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
    dirty(t);
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
    new.filetype = strrchr(filename, '.');
    /*
    time_t t = time(NULL);
    struct tm *time;
    time = localtime(&t);
    if (time == NULL) {
        die("localtime");
    }
    // .swp files are named as YYYY-MM-DD-(filename).swp
    new.swp = malloc(sizeof(char) * (17 + sizeof(new.filename)));
    strftime(new.swp, 12, ".%Y-%m-%d-", time);
    snprintf(new.swp, (17 + sizeof(new.filename)), "%s%s.swp", new.swp, new.filename);
    */
    new.syn = NULL;
    selectSyntaxHighlight(filename, new.filetype, new.syn);
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

    FILE *fptr = fopen(filename, "r");
    if (!fptr)
        die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fptr)) != -1) {
        while (linelen > 0 &&
               (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            linelen--;
        tabInsertRow(&new, new.numrows, line, linelen);
    }

    free(line);
    fclose(fptr);
    return new;
}

char *tabRowsToString(tab *t, int *buflen) {
    int totlen = 0;
    int j;
    for (j = 0; j < t->numrows; j++)
        totlen += t->rows[j].size + 1;
    *buflen = totlen;

    char *buf = malloc(totlen);
    char *p = buf;
    for (j = 0; j < t->numrows; j++) {
        memcpy(p, t->rows[j].chars, t->rows[j].size);
        p += t->rows[j].size;
        *p = '\n';
        p++;
    }

    return buf;
}

void dirty(tab *t) {
    t->dirty++;
    if (t->dirty % 5 == 0) {
        // Every five changes save
        tabBackup(t);
    }
}

void tabSave(tab *t) {
    if (t->filename) {
        // Save to filename
        int len;
        char *buf = tabRowsToString(t, &len);

        int fd = open(t->filename, O_RDWR | O_CREAT, 0644);
        if (fd != -1) {
            if (ftruncate(fd, len) != 1) {
                if (write(fd, buf, len) == len) {
                    close(fd);
                    free(buf);
                    t->dirty = 0;
                    setStatusMessage(t->bar, "%d bytes written to disk", len);
                    return;
                }
            }
            close(fd);
        }
        free(buf);
        setStatusMessage(t->bar, "Can't save! I/O error: %s", strerror(errno));
    }
}

void tabBackup(tab *t) {
    return;
    if (t->swp) {
        // Save to swap file
        int len;
        char *buf = tabRowsToString(t, &len);

        int fd = open(t->swp, O_RDWR | O_CREAT, 0644);
        if (fd != -1) {
            if (ftruncate(fd, len) != -1) {
                if (write(fd, buf, len) == len) {
                    close(fd);
                    free(buf);
                    return;
                }
            }
        }
        free(buf);
        setStatusMessage(t->bar, "Can't backup! I/O error: %s",
                         strerror(errno));
    }
}

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

void drawTab(tab *t, abuf *ab, colors theme) {
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
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
                       t->filename ? t->filename : "[No Name]", t->numrows,
                       t->dirty ? "(modified)" : "");
    int rlen =
        snprintf(rstatus, sizeof(rstatus), "%s | %d/%d",
                 t->filetype ? t->filetype : "no ft", t->cy + 1, t->numrows);
    if (len > t->screencols)
        len = t->screencols;
    abAppend(ab, status, len);
    while (len < t->screencols) {
        if (t->screencols - len == rlen) {
            abAppend(ab, rstatus, rlen);
            break;
        } else {
            abAppend(ab, " ", 1);
            len++;
        }
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

/*** Commands (:) ***/

void tabJumpTo(tab *t, int line) {
    if (line == 0)
        line = 1;
    else if (line > t->numrows)
        line = t->numrows;
    t->cy = line - 1;
}

void tabCommand(tab *t, char *buf, int key) {
    int jump = 1;
    for (unsigned long i = 0; i < strlen(buf); i++) {
        if (!isdigit(buf[i])) {
            jump = -1;
            break;
        }
    }

    if (jump == 1) {
        tabJumpTo(t, atoi(buf));
    } else {
        // TODO
        unsigned int i = 0;
        while (buf[i]) {
            char c = buf[i];
            if (c == 'w') {
                tabSave(t);
            } else if (c == 'q') {
                write(STDOUT_FILENO, "\x1b[2J", 4);
                write(STDOUT_FILENO, "\x1b[H", 3);
                exit(0);
            }
            i++;
        }
    }
}

/*** find ***/

void tabFindCallback(tab *t, char *query, int key) {
    // ! This needs to be rewritten because static won't work with multiple tabs
    static int last_match = -1;
    static int direction = 1;

    static int saved_hl_line;
    static char *saved_hl = NULL;

    if (saved_hl) {
        memcpy(t->rows[saved_hl_line].hl, saved_hl,
               t->rows[saved_hl_line].rsize);
        free(saved_hl);
        saved_hl = NULL;
    }

    if (key == '\r' || key == '\x1b') {
        last_match = -1;
        direction = 1;
        return;
    } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
        direction = 1;
    } else if (key == ARROW_LEFT || key == ARROW_UP) {
        direction = -1;
    } else {
        last_match = -1;
        direction = 1;
    }

    if (last_match == -1)
        direction = 1;
    int current = last_match;
    int i;
    for (i = 0; i < t->numrows; i++) {
        current += direction;
        if (current == -1)
            current = t->numrows - 1;
        else if (current == t->numrows)
            current = 0;

        row *r = &t->rows[current];
        char *match = strstr(r->render, query);
        if (match) {
            last_match = current;
            t->cy = current;
            t->cx = rowRxToCx(r, match - r->render);
            t->rowoff = t->numrows;

            /*
            saved_hl_line = current;
            saved_hl = malloc(r->rsize);
            memcpy(saved_hl, r->hl, r->rsize);
            memset(&r->hl[match - r->render], HL_MATCH, strlen(query));
            */
            break;
        }
    }
}

void tabFind(tab *t, void (*render)(void)) {
    int saved_cx = t->cx;
    int saved_cy = t->cy;
    int saved_coloff = t->coloff;
    int saved_rowoff = t->rowoff;

    tabPrompt(t, "/", render, tabFindCallback, 1);

    t->cx = saved_cx;
    t->cy = saved_cy;
    t->coloff = saved_coloff;
    t->rowoff = saved_rowoff;
}

/*** prompt ***/

char *tabPrompt(tab *t, char *prompt, void (*render)(void),
                void (*callback)(tab *t, char *, int), int onchange) {
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
            if (callback && onchange)
                callback(t, buf, c);
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

        if (callback && onchange)
            callback(t, buf, c);
    }
}

/*** modes ***/

int tabNormalMode(tab *t, int key, void (*render)(void)) {
    switch (key) {
    case CTRL_KEY('s'):
        tabSave(t);
        break;
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
        tabPrompt(t, ":", render, tabCommand, 0);
        break;
    case SLASH:
        tabFind(t, render);
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
        setStatusMessage(t->bar, "Switching to EDIT");
        return EDIT;
    }

    return NORMAL;
}

int tabEditMode(tab *t, int key, void (*render)(void)) {
    switch (key) {
    case ESC:
        setStatusMessage(t->bar, "Switching to NORMAL");
        return NORMAL;
    case CTRL_KEY('s'):
        tabSave(t);
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
        dirty(t);
        break;
    case '\r':
        tabInsertNewline(t);
        break;
    default:
        tabInsertChar(t, key);
        dirty(t);
        break;
    }
    return EDIT;
}

