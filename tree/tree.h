#ifndef TREE_H
#define TREE_H

#include "../libs/buffer.h"

typedef struct file {
    // Just makes things easier to work with
    char *name;
} file;

typedef struct tree {
    char *name;
    int numfolders;
    int numfiles;
    struct tree **folders;
    file *files;
    int rowoff;
} tree;

tree loadTree(char *dirname);

void drawTree(tree *t, abuf *ab, int y);

#endif
