#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../libs/buffer.h"

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

int editorReadKey(void);

typedef struct keypress {
    char associated;
    int key;
    struct keypress *next;
} keypress;

keypress *addKeystroke(int key, keypress *ptr, char associated);

keypress *lastKeystroke(keypress *ptr);

abuf stringKeystroke(keypress *ptr, int amt);

#endif
