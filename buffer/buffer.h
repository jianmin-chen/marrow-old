#ifndef BUFFER_H
#define BUFFER_H

#define ABUF_INIT                                                              \
    { NULL, 0 }

extern struct abuf {
    char *b;
    int len;
} abuf;

void abAppend(struct abuf *ab, const char *s, int len);

void abFree(struct abuf *ab);

#endif

