#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include "./config.h"
#include "./modes.h"
#include "./status/error.h"
#include "./status/status.h"
#include "./status/settings.h"
#include "./tab/tab.h"
#include "./tree/tree.h"
#include "./window/window.h"
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define MARROW_VERSION "0.0.5"

/*** prototypes ***/

void render(void);

typedef struct workspaceConfig {
    struct termios terminal;
    int activetab;
    int numtabs;
    tab *tabs;
    tree filetree;
    status bar;
    int keypress;
    int mode;
} workspaceConfig;

workspaceConfig global;

/*** manage tabs ***/

void workspaceInsertTab(int at, tab t) {
    global.tabs = realloc(global.tabs, sizeof(t) + sizeof(global.tabs));
    memmove(&global.tabs[at + 1], &global.tabs[at],
            sizeof(t) * (global.numtabs - at));
    global.tabs[at] = t;
}

void workspaceActiveTab(int at) { global.activetab = at; }

void workspaceResize(int sig) {
    if (SIGWINCH == sig) {
        resize(sig);

        tab *activeTab = &global.tabs[global.activetab];
        activeTab->screenrows = win.rows;
        activeTab->screencols = win.cols;

        render();
    }
}

void initWorkspace(void) {
    global.activetab = -1;
    global.numtabs = 0;
    global.tabs = NULL;
    global.filetree = loadTree(".");
    global.bar.statusmsg[0] = '\0';
    global.bar.statusmsg_time = 0;
    global.keypress = 0;
    global.mode = NORMAL;

    config = initSettings();

    if (getWindowSize(&win.rows, &win.cols) == -1)
        die("getWindowSize");
}

void render(void) {
    abuf ab = ABUF_INIT;

    // Hide cursor, move to top left
    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    // Render screen
    tab *activeTab = &global.tabs[global.activetab];

    tabScroll(activeTab);

    for (int y = 0; y < win.rows; y++) {
        drawTabLine(activeTab, &ab, y);
    }

    // Draw file status bar
    drawTabBar(activeTab, &ab);

    // Draw message bar (status bar, whatever you want to call it)
    drawStatusBar(&global.bar, &ab, win.cols);

    // Draw cursor
    drawTabCursor(activeTab, &ab);

    abAppend(&ab, "\x1b[?25h", 6); // Show cursor again

    write(STDIN_FILENO, (&ab)->b, (&ab)->len);
    abFree(&ab);
}

void process(int key) {
    switch (key) {
    case CTRL_KEY('q'):
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
    default:
        if (global.mode == NORMAL) {
            global.mode =
                tabNormalMode(&global.tabs[global.activetab], key, render);
        } else if (global.mode == EDIT) {
            global.mode =
                tabEditMode(&global.tabs[global.activetab], key, render);
        }
    }
}

void update(void) {
    tab *activeTab = &global.tabs[global.activetab];
    int linelen;
    if (config.lineNumbers) {
        linelen = floor(log10(activeTab->numrows + 1)) + 2;
        if (config.gitGutters)
            linelen++;
        activeTab->gutter = linelen + 2;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: marrow <file> [options]\n");
        return 0;
    }

    enableRawMode();
    initWorkspace();

    tab new = tabOpen(argv[1], win.rows, win.cols, &global.bar);
    workspaceInsertTab(0, new);
    workspaceActiveTab(0);

    signal(SIGWINCH, workspaceResize);
    update();

    while (1) {
        render();
        global.keypress = editorReadKey();
        process(global.keypress);
        update();
    }

    return 0;
}
