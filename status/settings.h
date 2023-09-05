#ifndef SETTINGS_H
#define SETTINGS_H

typedef struct settings {
    int hlHighlightNumbers;
    int hlHighlightStrings;
    char *hlHighlightLocation;

    int autoBackup;
    int backup;
    int center;
    int gitGutters;
    int lineNumbers;
    int softWrap;
    int tabStop;
} settings;

extern char **environ;
extern settings config;

settings initSettings(void);

#endif
