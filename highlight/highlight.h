#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

typedef struct colors {
    int background;
    int normal;
    int keyword;
    int type;
    int string;
    int number;
    int match;
    int comment;
} colors;

typedef struct syntax {
    char *filetype;
    char **filematch;
    char **keywords;
    char *singlelineCommentStart;
    char *multilineCommentStart;
    char *multilineCommentEnd;
    int flags;
} syntax;

enum highlight {
    HL_NORMAL = 0,
    HL_COMMENT,
    HL_MLCOMMENT,
    HL_KEYWORD,
    HL_TYPE,
    HL_STRING,
    HL_NUMBER,
    HL_MATCH
};

extern colors *theme;

int is_separator(int c);

int syntaxToColor(colors *theme, int hl);

syntax *selectSyntaxHighlight(char *filename, char *filetype);

#endif

