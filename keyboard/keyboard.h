#ifndef KEYBOARD_H
#define KEYBOARD_H

#define CTRL_KEY(k) ((k)&0x1f)

enum keys {
    ESC=27,
    DOLLAR=36,
    ZERO=48,
    COLON=58,
    H=104,
    I=105,
    J=106,
    K=107,
    L=108, 
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
