#ifndef TREE_H
#define TREE_H

typedef struct file {
    // Just makes things easier to work with
    char *name;
} file;

typedef struct tree {
    int numfolders;
    int numfiles;
    struct tree *folders;
    file *files;
} tree;

tree *loadTree(char *dirname);

#endif
