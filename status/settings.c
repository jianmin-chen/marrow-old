#include "../config.h"
#include <stdio.h>
#include <stdlib.h>

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

char **environ;
settings config;

settings initSettings(void) {
    settings s;
    s.hlHighlightNumbers = HL_HIGHLIGHT_NUMBERS;
    s.hlHighlightStrings = HL_HIGHLIGHT_STRINGS;
    s.hlHighlightLocation = HL_HIGHLIGHT_LOCATION;
    s.autoBackup = MARROW_AUTO_BACKUP;
    s.backup = MARROW_BACKUP;
    s.center = MARROW_CENTER;
    s.gitGutters = MARROW_GIT_GUTTERS;
    s.lineNumbers = MARROW_LINE_NUMBERS;
    s.softWrap = MARROW_SOFT_WRAP;
    s.tabStop = MARROW_TAB_STOP;
    return s;
}