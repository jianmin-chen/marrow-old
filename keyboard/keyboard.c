#include "../status/error.h"
#include "../libs/buffer.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#define CTRL_KEY(k) ((k)&0x1f)

enum keys {
    // Normal mode keys
    ESC = 27,
    DOLLAR = 36,
    SLASH = 47,
    ZERO = 48,
    COLON = 58,
    D = 100,
    H = 104,
    I = 105,
    J = 106,
    K = 107,
    L = 108,
    R = 114,
    U = 117,
    W = 119,
    Z = 122,
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

int editorReadKey(void) {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN)
            die("read");
    }

    if (c == '\x1b') {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1)
            return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1)
            return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1)
                    return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                    case '1':
                        return HOME_KEY;
                    case '3':
                        return DEL_KEY;
                    case '4':
                        return END_KEY;
                    case '5':
                        return PAGE_UP;
                    case '6':
                        return PAGE_DOWN;
                    case '7':
                        return HOME_KEY;
                    case '8':
                        return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                case 'A':
                    return ARROW_UP;
                case 'B':
                    return ARROW_DOWN;
                case 'C':
                    return ARROW_RIGHT;
                case 'D':
                    return ARROW_LEFT;
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
            case 'H':
                return HOME_KEY;
            case 'F':
                return END_KEY;
            }
        }

        return '\x1b';
    } else {
        return c;
    }
}

// Storing keypresses in a stack
typedef struct keypress {
    char associated;
    int key;
    struct keypress *next;
} keypress;

keypress *addKeystroke(int key, keypress *ptr, char associated) {
    keypress *k = malloc(sizeof(keypress));
    k->key = key;
    k->next = ptr;
    if (associated != NULL) k->associated = associated;
    else k->associated = key;
    return k;
}

keypress *lastKeystroke(keypress *ptr) {
    // Get last keystroke and then remove from stack
    keypress *k = ptr;
    ptr = ptr->next;
    return k; // Make sure to free this!
}

char *stringKeystroke(keypress *ptr) {
    // Convert stack of keystrokes to string
    abuf ab = ABUF_INIT;
    keypress *curr = ptr;
    int i = 0;
    while (curr != NULL) {
        int ndigits = floor(log10(curr->key));
        char buf[ndigits + 1];
        int clen = snprintf(buf, sizeof(buf), "%i,", curr->key);
        abAppend(&ab, buf, clen);
        curr = curr->next;
    }
    abAppend(&ab, "\0", 1);

    return ab.b;
}

