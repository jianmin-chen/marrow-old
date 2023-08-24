#include <stdio.h>
#include <stdlib.h>
#include "../config.h"

extern char **environ;

typedef struct settings {
    int hl_highlight_numbers;
    int hl_highlight_strings;
    char *hl_highlight_location;

    int tab_stop;
    int quit_times;
    int line_numbers;
    int git_gutters;
    int center;
    int soft_wrap;
} settings;

settings initSettings() {
    settings s;
    s.hl_highlight_numbers = HL_HIGHLIGHT_NUMBERS;
    s.hl_highlight_strings = HL_HIGHLIGHT_STRINGS;
    s.hl_highlight_location = HL_HIGHLIGHT_LOCATION;
    s.tab_stop = MARROW_TAB_STOP;
    s.quit_times = MARROW_QUIT_TIMES;
    s.line_numbers = MARROW_LINE_NUMBERS;
    s.git_gutters = MARROW_GIT_GUTTERS;
    s.center = MARROW_CENTER;
    s.soft_wrap = MARROW_SOFT_WRAP;
    return s;
}