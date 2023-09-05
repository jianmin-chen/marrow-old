#include "../keyboard/keyboard.h"
#include "../libs/buffer.h"
#include "../status/settings.h"
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>

#define SHELL "/bin/marrow"

typedef struct row {
    int idx;
    int size;
    char *chars;
} row;

typedef struct emulator {
    int master, slave;
    int numrows;
    row *rows;
    int cx, cy;
    int rowoff;
    int screenrows, screencols;
} emulator;

void pt_pair(emulator *e) {
    char *slave_name;

    e->master = posix_openpt(O_RDWR | O_NOCTTY);
    if (e->master == -1)
        die("posix_openpt");

    /*
     * grantpt() and unlockpt() are housekeeping functions that have to
     * be called before we can open the slave FD. Refer to the manpages
     * on what they do.
     */
    if (grantpt(e->master) == -1)
        die("grantpt");
    if (unlockpt(e->master) == -1)
        die("unlockpt");

    /*
     * Up until now, we only have the master FD. We also need a file
     * descriptor for our child process. We get it by asking for the
     * actual path in /dev/pts which we then open using a regular
     * open(). So, unlike pipe(), you don't get two corresponding file
     * descriptors in one go.
     */

    slave_name = ptsname(e->master);
    if (slave_name == NULL)
        die("ptsname");

    e->slave = open(slave_name, O_RDWR | O_NOCTTY);
    if (e->slave == -1)
        die("open(slave_name)");
}

void spawn(emulator *e) {
    pid_t pid;

    pid = fork();
    if (pid == 0) {
        close(e->master);

        /*
         * Create a new session and make our terminal this process'
         * controlling terminal. The shell that we'll spawn in a second
         * will inherit the status of session leader.
         */
        setsid();
        if (ioctl(e->slave, TIOCSCTTY, NULL) == -1)
            die("ioctl(TIOCSCTTY)");

        dup2(e->slave, 0);
        dup2(e->slave, 1);
        dup2(e->slave, 2);
        close(e->slave);

        execle(SHELL, "-" SHELL, (char *)NULL, environ);
    } else if (pid > 0)
        close(e->slave);
    die("fork");
}

void emulatorRowInsertChar(emulator *e, row *r, int at, int c) {
    if (at < 0 || at > r->size)
        at = r->size;
    r->chars = realloc(r->chars, r->size + 2);
    memmove(&r->chars[at + 1], &r->chars[at], r->size - at * 1);
    r->size++;
    r->chars[at] = c;
}

void emulatorRowDelChar(emulator *e, row *r, int at) {
    if (at < 0 || at >= r->size)
        return;
    memmove(&r->chars[at], &r->chars[at + 1], r->size - at);
    r->size--;
}

void emulatorRowAppendString(emulator *e, row *r, char *s, size_t len) {
    r->chars = realloc(r->chars, r->size + len + 1);
    memcpy(&r->chars[r->size], s, len);
    r->size += len;
    r->chars[r->size] = '\0';
}

void emulatorInsertRow(emulator *e, int at, char *s, size_t len) {
    if (at < 0 || at > e->numrows)
        return;

    e->rows = realloc(e->rows, sizeof(row) * (e->numrows + 1));
    memmove(&e->rows[at + 1], &e->rows[at], sizeof(row) * (e->numrows - at));

    for (int j = at + 1; j <= e->numrows; j++)
        e->rows[j].idx++;

    e->rows[at].idx = at;
    e->rows[at].size = len;
    e->rows[at].chars = malloc(len + 1);
    memcpy(e->rows[at].chars, s, len);
    e->rows[at].chars[len] = '\0';

    e->numrows++;
}

void emulatorFreeRow(row *r) { free(r->chars); }

void emulatorDelRow(emulator *e, int at) {
    if (at < 0 || at >= e->numrows)
        return;
    emulatorFreeRow(&e->rows[at]);
    memmove(&e->rows[at], &e->rows[at + 1],
            sizeof(row) * (e->numrows - at - 1));
    for (int j = at; j < e->numrows - 1; j++)
        e->rows[j].idx--;
    e->numrows--;
}

void emulatorInsertNewline(emulator *e) {
    if (e->cx == 0) {
        emulatorInsertRow(e, e->cy, "", 0);
    } else {
        row *r = &e->rows[e->cy];
        emulatorInsertRow(e, e->cy + 1, &r->chars[e->cx], r->size - e->cx);

        // Update previous row
        r = &e->rows[e->cy];
        r->size = e->cx;
        r->chars[r->size] = '\0';
    }
    e->cy++;
    e->cx = 0;
}

void emulatorInsertChar(emulator *e, int c) {
    if (e->cy == e->numrows)
        emulatorInsertRow(e, e->numrows, "", 0);
    emulatorRowInsertChar(e, &e->rows[e->cy], e->cx, c);
    e->cx++;
}

void emulatorDelChar(emulator *e) {
    if (e->cy == e->numrows)
        return;

    row *r = &e->rows[e->cy];
    if (e->cx > 0) {
        emulatorRowDelChar(e, r, e->cx - 1);
        e->cx--;
    } else {
        e->cx = e->rows[e->cy - 1].size;
        emulatorRowAppendString(e, &e->rows[e->cy - 1], r->chars, r->size);
        emulatorDelRow(e, e->cy);
        e->cy--;
    }
}

emulator createEmulator(int screenrows, int screencols) {
    emulator e;
    e.numrows = 0;
    e.rows = NULL;
    e.cx = 0;
    e.cy = 0;
    e.rowoff = 0;
    e.screenrows = screenrows;
    e.screencols = screencols;
    pt_pair(&e);
    return e;
}

void emulatorScroll(emulator *e) {
    if (e->cy < e->rowoff)
        e->rowoff = e->cy;
    else if (e->cy >= e->rowoff + e->screenrows)
        e->rowoff = e->cy - e->screenrows + 1;
}

void drawEmulatorCursor(emulator *e, abuf *ab) {
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (e->cy - e->rowoff) + 1,
             e->cx + 1);
    abAppend(ab, buf, strlen(buf));
}

void drawEmulatorLine(emulator *e, abuf *ab, int y) {
    int filerow = y + e->rowoff;
    if (filerow >= e->numrows) {
        abAppend(ab, "~", 1);
        abAppend(ab, "\x1b[K", 3);
        abAppend(ab, "\r\n", 2);
        return;
    }
    int len = e->rows[filerow].size;
    char *c = e->rows[filerow].chars;
    int j;
    for (j = 0; j < len; j++) {
        abAppend(ab, &c[j], 1);
    }
    abAppend(ab, "\x1b[K", 3);
    abAppend(ab, "\r\n", 2);
}

void emulatorMoveCursor(emulator *e, int key) {
    row *r = (e->cy >= e->numrows) ? NULL : &e->rows[e->cy];

    switch (key) {
    case ARROW_LEFT:
        if (e->cx != 0)
            e->cx--;
        else if (e->cy > 0) {
            e->cy--;
            e->cx = e->rows[e->cy].size;
        }
        break;
    case ARROW_RIGHT:
        if (r && e->cx < r->size)
            e->cx++;
        else if (r && e->cx == r->size) {
            e->cy++;
            e->cx = 0;
        }
        break;
    case ARROW_UP:
        if (e->cy != 0)
            e->cy--;
        break;
    case ARROW_DOWN:
        if (e->cy < e->numrows)
            e->cy++;
        break;

        r = (e->cy >= e->numrows) ? NULL : &e->rows[e->cy];
        int rowlen = r ? r->size : 0;
        if (e->cx > rowlen)
            e->cx = rowlen;
    }
}

void emulatorReadKey(emulator *e, int key) {
    write(e->master, key, 1);
    switch (key) {
    case HOME_KEY:
        e->cx = 0;
        break;
    case END_KEY:
        if (e->cy < e->numrows)
            e->cx = e->rows[e->cy].size;
        break;
    case PAGE_UP:
    case PAGE_DOWN: {
        if (key == PAGE_UP)
            e->cy = e->rowoff;
        else if (key == PAGE_DOWN) {
            e->cy = e->rowoff + e->screenrows - 1;
            if (e->cy > e->numrows)
                e->cy = e->numrows;
        }
    } break;
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
        emulatorMoveCursor(e, key);
        break;
    case '\r':
        emulatorInsertNewline(e);
        break;
    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
        if (key == DEL_KEY)
            emulatorMoveCursor(e, ARROW_RIGHT);
        emulatorDelChar(e);
        break;
    default:
        emulatorInsertChar(e, key);
        break;
    }
}
