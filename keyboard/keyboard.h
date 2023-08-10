#ifndef KEYBOARD_H
#define KEYBOARD_H

#define CTRL_KEY(k) ((k)&0x1f)

enum keys {
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

#endif

