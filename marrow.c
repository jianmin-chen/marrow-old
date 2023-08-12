#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include "./config.h"
#include "./highlight/highlight.h"
#include "./keyboard/keyboard.h"
#include "./libs/buffer.h"
#include "./modes.h"
#include "./status/error.h"
#include "./status/status.h"
#include "./tab/tab.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define MARROW_VERSION "0.0.3"

typedef struct workspaceConfig {
    struct termios terminal;
    int activetab;
    int numtabs;
    tab *tabs;
    status bar;
    int rows;
    int cols;
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

/*** deal with modes ***/

void disableRawMode(void) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &global.terminal) == -1)
        die("tcsetattr");
}

void enableRawMode(void) {
    if (tcgetattr(STDIN_FILENO, &global.terminal) == -1)
        die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = global.terminal;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}

int getCursorPosition(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
        return -1;

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1)
            break;
        if (buf[i] == 'R')
            break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[')
        return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
        return -1;

    return 0;
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        // Get window size the hard way
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
            return -1;
        return getCursorPosition(rows, cols);
    } else {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
        return 0;
    }
}

void initWorkspace(void) {
    global.activetab = -1;
    global.numtabs = 0;
    global.tabs = NULL;
    global.bar.statusmsg[0] = '\0';
    global.bar.statusmsg_time = 0;
    global.rows = 0;
    global.cols = 0;
    global.keypress = 0;
    global.mode = NORMAL;

    if (getWindowSize(&global.rows, &global.cols) == -1)
        die("getWindowSize");

    global.rows -= 2;
}

void render(void) {
    abuf ab = ABUF_INIT;

    // Hide cursor, move to top left
    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    // Render screen
    if (global.activetab == -1) {
        int y;
        for (y = 0; y < global.rows; y++) {
            if (y == global.rows / 2) {
                char welcome[80];
                int welcomelen =
                    snprintf(welcome, sizeof(welcome),
                             "Marrow editor -- version %s", MARROW_VERSION);
                                
                    welcomelen = global.cols;
                int padding = (global.cols - welcomelen) / 2;

                if (padding) {
                    abAppend(&ab, "~", 1);
                    padding--;
                }
                while (padding--)
                    abAppend(&ab, " ", 1);
                abAppend(&ab, welcome, welcomelen);
            } else
                abAppend(&ab, "~", 1);
            abAppend(&ab, "\x1b[K", 3);
            abAppend(&ab, "\r\n", 2);
        }
    } else {
        tab *activeTab = &global.tabs[global.activetab];

        drawTab(activeTab, &ab);

        // Draw message bar (status bar, whatever you want to call it)
        drawStatusBar(&global.bar, &ab, global.cols);

        // Draw cursor
        char buf[32];
        snprintf(buf, sizeof(buf), "\x1b[%d;%dH",
                 (activeTab->cy - activeTab->rowoff) + 1,
                 (activeTab->rx - activeTab->coloff) + 1);
        abAppend(&ab, buf, strlen(buf));
    }

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

void update(void) {}

int main(int argc, char *argv[]) {
    enableRawMode();
    initWorkspace();

    if (argc >= 2) {
        tab new = tabOpen(argv[1], global.rows, global.cols, &global.bar);
        workspaceInsertTab(0, new);
        workspaceActiveTab(0);
    }

    while (1) {
        render();
        global.keypress = editorReadKey();
        process(global.keypress);
        update();
    }

    return 0;
}

