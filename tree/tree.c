#include <dirent.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libs/buffer.h"
#include "../status/error.h"

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

/*** prototypes ***/

tree *loadTree(char *dirname);

void treeAddFolder(tree *t, char *foldername) {
}

void treeAddFile(tree *t, char *filename) {
    file *f;
    f->name = filename;
}

tree *loadTree(char *dirname) {
    tree *t;
    t->numfolders = 0;
    t->numfiles = 0;
    t->folders = NULL;
    t->files = NULL;

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(dirname)) != NULL) {
        // Get all the files and directories within
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_DIR) treeAddFolder(t, ent->d_name);
            else treeAddFile(t, ent->d_name);
        }
        closedir(dir);
    } else {
        die("opendir");
    }
    return t;
}
