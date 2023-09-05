#include "../config.h"
#include "../highlight/highlight.h"
#include "../keyboard/keyboard.h"
#include "../libs/re.h"
#include "../modes.h"
#include "../status/error.h"
#include "../status/status.h"
#include "../status/settings.h"
#include "../window/window.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

/*** prototypes ***/

void tabUpdateRow(tab *t, row *r);
void dirty(tab *t);
int tabCenter(tab *t, char *buf, int key);

/*** row operations ***/

int rowCxToRx(row *r, int cx) {
    int rx = 0;
    int j;
    for (j = 0; j < cx; j++) {
        if (r->chars[j] == '\t')
            rx += (config.tabStop - 1) - (rx % config.tabStop);
        rx++;
    }
    return rx;
}

int rowRxToCx(row *r, int rx) {
    int cur_rx = 0;
    int cx;
    for (cx = 0; cx < r->size; cx++) {
        if (r->chars[cx] == '\t')
            cur_rx += (config.tabStop - 1) - (cur_rx % config.tabStop);
        cur_rx++;
        if (cur_rx > rx)
            return cx;
    }
    return cx;
}

void tabRowInsertChar(tab *t, row *r, int at, int c) {
    if (at < 0 || at > r->size)
        at = r->size;
    r->chars = realloc(r->chars, r->size + 2);
    memmove(&r->chars[at + 1], &r->chars[at], r->size - at * 1);
    r->size++;
    r->chars[at] = c;
    tabUpdateRow(t, r);
}

void tabRowDelChar(tab *t, row *r, int at) {
    if (at < 0 || at >= r->size)
        return;
    memmove(&r->chars[at], &r->chars[at + 1], r->size - at);
    r->size--;
    tabUpdateRow(t, r);
}

void tabRowAppendString(tab *t, row *r, char *s, size_t len) {
    r->chars = realloc(r->chars, r->size + len + 1);
    memcpy(&r->chars[r->size], s, len);
    r->size += len;
    r->chars[r->size] = '\0';
    tabUpdateRow(t, r);
}

/*** syntax related row operations ***/

void tabUpdateSyntax(tab *t, row *r) {
    if (!t->syn)
        return;
    r->hl = realloc(r->hl, r->rsize);
    memset(r->hl, HL_NORMAL, r->rsize);

    char **keywords = t->syn->keywords;

    char *scs = t->syn->singlelineCommentStart;
    char *mcs = t->syn->multilineCommentStart;
    char *mce = t->syn->multilineCommentEnd;

    int scs_len = scs ? strlen(scs) : 0;
    int mcs_len = mcs ? strlen(mcs) : 0;
    int mce_len = mce ? strlen(mce) : 0;

    int prev_sep = 1;
    int in_string = 0;
    int in_comment = (r->idx > 0 && t->rows[r->idx - 1].hl_open_comment);

    int i = 0;
    while (i < r->rsize) {
        char c = r->render[i];
        unsigned char prev_hl = (i > 0) ? r->hl[i - 1] : HL_NORMAL;

        /* Comments */

        if (scs_len && !in_string && !in_comment) {
            if (!strncmp(&r->render[i], scs, scs_len)) {
                memset(&r->hl[i], HL_COMMENT, r->rsize - i);
                break;
            }
        }

        if (mcs_len && mce_len && !in_string) {
            if (in_comment) {
                r->hl[i] = HL_MLCOMMENT;
                if (!strncmp(&r->render[i], mce, mce_len)) {
                    memset(&r->hl[i], HL_MLCOMMENT, mce_len);
                    i += mce_len;
                    in_comment = 0;
                    prev_sep = 1;
                    continue;
                } else {
                    i++;
                    continue;
                }
            } else if (!strncmp(&r->render[i], mcs, mcs_len)) {
                memset(&r->hl[i], HL_MLCOMMENT, mcs_len);
                i += mcs_len;
                in_comment = 1;
                continue;
            }
        }

        /* Strings */

        if (t->syn->flags & config.hlHighlightStrings) {
            if (in_string) {
                r->hl[i] = HL_STRING;
                if (c == '\\' && i + 1 < r->rsize) {
                    r->hl[i + 1] = HL_STRING;
                    i += 2;
                    continue;
                }
                if (c == in_string)
                    in_string = 0;
                i++;
                prev_sep = 1;
                continue;
            } else if (c == '"' || c == '\'') {
                in_string = c;
                r->hl[i] = HL_STRING;
                i++;
                continue;
            }
        }

        /* Numbers */

        if (t->syn->flags & config.hlHighlightNumbers) {
            if (isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) {
                r->hl[i] = HL_NUMBER;
                i++;
                if (!strncmp(&r->render[i], ".", 1)) {
                    r->hl[i] = HL_NUMBER;
                    i++;
                }
                prev_sep = 0;
                continue;
            }
        }

        /* Keywords */

        if (prev_sep) {
            int j;
            for (j = 0; keywords[j]; j++) {
                int klen = strlen(keywords[j]);
                int type = keywords[j][klen - 1] == '|';
                if (type)
                    klen--;

                if (!strncmp(&r->render[i], keywords[j], klen) &&
                    is_separator(r->render[i + klen])) {
                    memset(&r->hl[i], type ? HL_TYPE : HL_KEYWORD, klen);
                    i += klen;
                    break;
                }
            }
            if (keywords[j] != NULL) {
                prev_sep = 0;
                continue;
            }
        }

        prev_sep = is_separator(c);
        i++;
    }

    int changed = (r->hl_open_comment != in_comment);
    r->hl_open_comment = in_comment;
    if (changed && r->idx + 1 < t->numrows) {
        tabUpdateSyntax(t, &t->rows[r->idx + 1]);
    }
}

/*** tab operations ***/

void tabUpdateRow(tab *t, row *r) {
    int tabs = 0;
    int j;
    for (j = 0; j < r->size; j++)
        if (r->chars[j] == '\t')
            tabs++;

    free(r->render);
    r->render = malloc(r->size + tabs * (config.tabStop - 1) + 1);

    int idx = 0;
    for (j = 0; j < r->size; j++) {
        if (r->chars[j] == '\t') {
            r->render[idx++] = ' ';
            while (idx % config.tabStop != 0)
                r->render[idx++] = ' ';
        } else {
            r->render[idx++] = r->chars[j];
        }
    }
    r->render[idx] = '\0';
    r->rsize = idx;
    tabUpdateSyntax(t, r);
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
    t->rows[at].hl = NULL;
    t->rows[at].hl_open_comment = 0;
    t->rows[at].changed = 0;

    tabUpdateRow(t, &t->rows[at]);

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
        t->cx = 0;
        if (config.gitGutters)
            t->rows[t->cy].changed = 1;
    } else {
        // Get number of tabs on current line and inject in
        row *r = &t->rows[t->cy];

        int spaces = 0;
        char *c = &r->render[0];
        while (c[spaces] == ' ')
            spaces++;

        // TODO: Python & other indented languages require you to use one
        // variant (tabs or spaces) so this needs to be fixed
        int size = r->size - t->cx;
        char *new;
        if (config.tabStop) {
            int tabs = spaces / config.tabStop;
            size += tabs;
            spaces = spaces % config.tabStop;
            size += spaces;
            new = malloc(sizeof(char) * size);
            for (int i = 0; i < tabs; i++)
                strcat(new, "\t");
            for (int i = 0; i < spaces; i++)
                strcat(new, " ");
        } else {
            size += spaces;
            new = malloc(sizeof(char) * size);
            for (int i = 0; i < spaces; i++)
                strcat(new, " ");
        }
        strcat(new, &r->render[t->cx]);
        new[size] = '\0';

        tabInsertRow(t, t->cy + 1, new, size);

        // Update previous row
        r = &t->rows[t->cy];
        r->size = t->cx;
        r->chars[r->size] = '\0';
        if (config.gitGutters)
            r->changed = 1;
        tabUpdateRow(t, r);

        t->cx = size;
    }
    t->cy++;
}

void tabInsertChar(tab *t, int c) {
    if (t->cy == t->numrows) {
        tabInsertRow(t, t->numrows, "", 0);
    }
    tabRowInsertChar(t, &t->rows[t->cy], t->cx, c);
    t->cx++;
    if (config.gitGutters) {
        // Mark as changed
        t->rows[t->cy].changed = 1;
    }
}

void tabDelChar(tab *t) {
    if (t->cy == t->numrows)
        return;

    row *r = &t->rows[t->cy];
    if (t->cx > 0) {
        tabRowDelChar(t, r, t->cx - 1);
        t->cx--;
    } else {
        t->cx = t->rows[t->cy - 1].size;
        tabRowAppendString(t, &t->rows[t->cy - 1], r->chars, r->size);
        tabDelRow(t, t->cy);
        t->cy--;
    }
}

/*** file operations ***/

tab tabOpen(char *filename, int screenrows, int screencols, status *s) {
    tab new;
    new.filename = strdup(filename);
    new.filetype = strrchr(filename, '.');

    if (config.backup) {
        time_t t = time(NULL);
        struct tm *time;
        time = localtime(&t);
        if (time == NULL)
            die("localtime");
        new.swp = malloc(sizeof(char) * (17 + strlen(filename)));
        strftime(new.swp, 12, ".%Y-%m-%d-", time);
        strcat(new.swp, new.filename);
        strcat(new.swp, ".swp\0");
    }

    new.syn = selectSyntaxHighlight(new.filename, new.filetype);
    new.keystrokes = NULL;
    new.numrows = 0;
    new.rows = NULL;
    new.bar = s;
    new.cx = 0;
    new.cy = 0;
    new.rx = 0;
    new.gutter = 0;
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

void tabSave(tab *t, int notify) {
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
                    if (notify)
                        setStatusMessage(t->bar, "%d bytes written to disk",
                                         len);
                    return;
                }
            }
            close(fd);
        }
        free(buf);
        if (notify)
            setStatusMessage(t->bar, "Can't save! I/O error: %s",
                             strerror(errno));
    }
}

void tabBackup(tab *t) {
    FILE *fptr = fopen(t->swp, "a");
    if (fptr == NULL)
        die("fopen");
    abuf output = stringKeystroke(t->keystrokes, 10);
    for (int i = 0; i < output.len; i++) {
        fprintf(fptr, "%c", output.b[i]);
    }
    fclose(fptr);
}

void dirty(tab *t) {
    t->dirty++;
    if (t->dirty % 10 == 0) {
        // Every five changes save
        tabBackup(t);
        if (config.autoBackup) {
            // Also autobackup to file
            tabSave(t, 0);
        }
    }
}

void tabScroll(tab *t) {
    t->rx = t->cx;
    if (t->cy < t->numrows)
        t->rx = rowCxToRx(&t->rows[t->cy], t->cx);

    if (config.center)
        tabCenter(t, "z", 0);
    else if (t->cy < t->rowoff)
        t->rowoff = t->cy;
    else if (t->cy >= t->rowoff + t->screenrows)
        t->rowoff = t->cy - t->screenrows + 1;
    if (t->rx < t->coloff)
        t->coloff = t->rx;
    if (t->rx >= t->coloff + t->screencols - t->gutter)
        t->coloff = t->rx - t->screencols + t->gutter + 1;
}

void drawTabCursor(tab *t, abuf *ab) {
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (t->cy - t->rowoff) + 1,
             (t->rx - t->coloff + t->gutter) + 1);
    abAppend(ab, buf, strlen(buf));
}

void drawTabLine(tab *t, abuf *ab, int y) {
    int filerow = y + t->rowoff;
    if (filerow >= t->numrows) {
        if (t->gutter) {
            for (int i = 0; i < t->gutter - 2; i++) {
                abAppend(ab, " ", 1);
            }
        }
        abAppend(ab, "~", 1);
        abAppend(ab, "\x1b[K", 3);
        abAppend(ab, "\r\n", 2);
        return;
    }

    int linelen = t->gutter - 1;
    int padding = 0;

    if (config.gitGutters && t->rows[filerow].changed) {
        abAppend(ab, "~", 2);
        padding += 2;
    }

    if (config.lineNumbers) {
        // Line numbers in gutter
        int ndigits = floor(log10(filerow + 1)) + 2;
        char buf[ndigits + 1];
        padding += ndigits;
        while (padding <= linelen) {
            abAppend(ab, " ", 1);
            padding++;
        }
        int clen = snprintf(buf, sizeof(buf), "%i ", filerow + 1);
        abAppend(ab, buf, clen);
    }

    if (t->syn) {
        // Syntax highlighting enabled
        int len = t->rows[filerow].rsize - t->coloff;
        if (len < 0)
            len = 0;
        if (len > t->screencols - t->gutter)
            len = t->screencols - t->gutter;
        char *c = &t->rows[filerow].render[t->coloff];
        unsigned char *hl = &t->rows[filerow].hl[t->coloff];
        int current_color = -1;
        int j;
        for (j = 0; j < len; j++) {
            if (iscntrl(c[j])) {
                // Control character, highlight differently
                char sym = (c[j] <= 26 ? '@' + c[j] : '?');
                abAppend(ab, "\x1b[7m", 4);
                abAppend(ab, &sym, 1);
                abAppend(ab, "\x1b[m", 3);
                if (current_color != -1) {
                    char buf[16];
                    int clen =
                        snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
                    abAppend(ab, buf, clen);
                }
            } else {
                int color = syntaxToColor(theme, hl[j]);
                if (color != current_color) {
                    current_color = color;
                    char buf[16];
                    int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
                    abAppend(ab, buf, clen);
                }
                abAppend(ab, &c[j], 1);
            }
        }
        abAppend(ab, "\x1b[39m", 5);
    } else {
        // No syntax highlighting
        int len = t->rows[filerow].rsize - t->coloff;
        if (len < 0)
            len = 0;
        if (len > t->screencols - t->gutter)
            len = t->screencols - t->gutter;
        char *c = &t->rows[filerow].render[t->coloff];
        int j;
        for (j = 0; j < len; j++) {
            abAppend(ab, &c[j], 1);
        }
    }

    abAppend(ab, "\x1b[K", 3);
    abAppend(ab, "\r\n", 2);
}

void drawTabBar(tab *t, abuf *ab) {
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
    t->cx = 0;
}

int tabCenter(tab *t, char *buf, int key) {
    if (strcmp(buf, "z") == 0) {
        t->rowoff = t->cy - (t->screenrows / 2);
        if (t->rowoff < 0)
            t->rowoff = 0;
        return 0;
    }

    return 1;
}

int tabDelete(tab *t, char *buf, int key) {
    if (strcmp(buf, "d") == 0) {
        tabDelRow(t, t->cy);
        return 0;
    }

    return 1;
}

int tabCommand(tab *t, char *buf, int key) {
    int jump = 1;
    for (unsigned long i = 0; i < strlen(buf); i++) {
        if (!isdigit(buf[i])) {
            jump--;
            break;
        }
    }

    if (jump == 1) {
        tabJumpTo(t, atoi(buf));
    } else {
        unsigned int i = 0;

        if (strncmp(buf, "settheme", 9) == 0 && strlen(buf) > 10) {
            char *theme = malloc(strlen(buf) - 9);
            for (unsigned int j = 9; j < strlen(buf); j++) {
                theme[j - 9] = buf[j];
            }
            loadTheme(theme);
            return 0;
        }

        while (buf[i]) {
            char c = buf[i];
            if (c == 'w') {
                tabSave(t, 1);
            } else if (c == 'q') {
                write(STDOUT_FILENO, "\x1b[2J", 4);
                write(STDOUT_FILENO, "\x1b[H", 3);
                disableRawMode();
                exit(0);
            }
            i++;
        }
    }

    return 0;
}

/*** find ***/

int tabFind(tab *t, char *query, int key) {
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
        return 0;
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

    re_t pattern = re_compile(query);
    int match_length;
    int match_idx;

    for (i = 0; i < t->numrows; i++) {
        current += direction;
        if (current == -1)
            current = t->numrows - 1;
        else if (current == t->numrows)
            current = 0;

        row *r = &t->rows[current];
        match_idx = re_matchp(pattern, r->render, &match_length);

        if (match_idx != -1) {
            last_match = current;
            t->cy = current;
            t->cx = rowRxToCx(r, &r->render[match_idx] - r->render);
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

    return 1;
}

/*** prompt ***/

char *tabPrompt(tab *t, char *prompt, void (*render)(void),
                int (*callback)(tab *t, char *, int), int onchange) {
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
            if (callback && onchange) {
                int response = callback(t, buf, c);
                if (response == 0) {
                    setStatusMessage(s, "");
                    return buf;
                }
            }
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

        if (callback && onchange) {
            int response = callback(t, buf, c);
            if (response == 0) {
                setStatusMessage(s, "");
                return buf;
            }
        }
    }
}

/*** modes ***/

void tabUndo(tab *t) {
    // "Mode" for undoing but not really
    return;
    keypress *last = lastKeystroke(t->keystrokes);
    switch (last->key) {
    case HOME_KEY:
    case END_KEY:
    case PAGE_UP:
    case PAGE_DOWN:
        break;
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
        tabMoveCursor(t, last->key);
        break;
    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
        if (last->key == DEL_KEY)
            tabMoveCursor(t, ARROW_LEFT);
    }
}

int tabNormalMode(tab *t, int key, void (*render)(void)) {
    switch (key) {
    case CTRL_KEY('s'):
        tabSave(t, 1);
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
    case D:
        tabPrompt(t, "d", render, tabDelete, 1);
        break;
    case R:
        break;
    case U:
        tabUndo(t);
        break;
    case Z:
        tabPrompt(t, "z", render, tabCenter, 1);
        break;
    case COLON:
        tabPrompt(t, ":", render, tabCommand, 0);
        break;
    case SLASH:
        tabPrompt(t, "/", render, tabFind, 1);
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
        tabSave(t, 1);
        return EDIT;
    case HOME_KEY:
        t->cx = 0;
        break;
    case END_KEY:
        if (t->cy < t->numrows)
            t->cx = t->rows[t->cy].size;
        break;
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
    case '\r':
        tabInsertNewline(t);
        break;
    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
        if (key == DEL_KEY)
            tabMoveCursor(t, ARROW_RIGHT);
        tabDelChar(t);
        dirty(t);
        break;
    default:
        tabInsertChar(t, key);
        dirty(t);
        break;
    }

    t->keystrokes = addKeystroke(key, t->keystrokes, '\0');

    return EDIT;
}
