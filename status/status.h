#ifndef STATUS_H
#define STATUS_H

#include "../libs/buffer.h"
#include <time.h>

typedef struct status {
    char statusmsg[80];
    time_t statusmsg_time;
} status;

void setStatusMessage(status *s, const char *fmt, ...);

char *statusPrompt(status *s, char *prompt, void (*render)(void),
                   void (*callback)(status *s, char *, int));

void drawStatusBar(status *s, abuf *ab, int screencols);

#endif
