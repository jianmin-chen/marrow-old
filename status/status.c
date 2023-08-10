#include "../buffer/buffer.h"
#include "../keyboard/keyboard.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct status {
    char statusmsg[80];
    time_t statusmsg_time;
} status;

void setStatusMessage(status *s, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s->statusmsg, sizeof(s->statusmsg), fmt, ap);
    va_end(ap);
    s->statusmsg_time = time(NULL);
}

char *statusPrompt(status *s, char *prompt, void (*render)(void),
                   void (*callback)(status *s, char *, int)) {
    size_t bufsize = 128;
    char *buf = malloc(bufsize);

    size_t buflen = 0;
    buf[0] = '\0';

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
                    callback(s, buf, c);
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

void drawStatusBar(status *s, abuf *ab, int screencols) {
    abAppend(ab, "\x1b[K", 3);
    int msglen = strlen(s->statusmsg);
    if (msglen > screencols)
        msglen = screencols;
    if (msglen && time(NULL) - s->statusmsg_time < 5)
        abAppend(ab, s->statusmsg, msglen);
}

