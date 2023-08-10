#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

#define HL_NUMBERS (1 << 0)
#define HL_STRINGS (1 << 1)

typedef struct syntax {
    char *filetype;
    char **filematch;
    char **keywords;
    char *singlelineCommentStart;
    char *multilineCommentStart;
    char *multilineCommentEnd;
    int flags;
} syntax;

syntax HLDB[];

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

#endif
