#include "../libs/buffer.h"
#include "../status/error.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct file {
    char *name;
} file;

typedef struct tree {
    char *name;
    int numfolders;
    int numfiles;
    struct tree *folders;
    file *files;
    int rowoff;
    int open;
} tree;

tree loadTree(char *dirname);

void treeAddFolder(tree *t, char *foldername) {
    // Initialize it to nothing to save on computing
    t->numfolders++;
    tree folder;
    folder.name = "";
    folder.numfolders = 0;
    folder.numfiles = 0;
    folder.folders = NULL;
    folder.files = NULL;
    folder.open = 0;
}

void treeAddFile(tree *t, char *filename) {
    file f;
    f.name = filename;
    int oldsize = sizeof(t->files);
    t->files = realloc(t->files, oldsize + sizeof(f));
    memmove(&t->files[t->numfiles], &f, sizeof(f));
    t->numfiles++;
}

tree loadTree(char *dirname) {
    tree t;
    t.name = dirname;
    t.numfolders = 0;
    t.folders = NULL;
    t.files = NULL;
    t.rowoff = 0;
    t.open = 1;

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(dirname)) != NULL) {
        // Get all the files and directories within
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_DIR)
                treeAddFolder(&t, ent->d_name);
            else
                treeAddFile(&t, ent->d_name);
        }
        closedir(dir);
    } else {
        die("opendir");
    }
    return t;
}
