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
    char *name;
} file;

typedef struct tree {
    char *name;
    int numfolders;
    int numfiles;
    struct tree *folders;
    file *files;
    int rowoff;
} tree;

/*** prototypes ***/

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
    t.numfiles = 0;
    t.folders = NULL;
    t.files = NULL;
    t.rowoff = 0;

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(dirname)) != NULL) {
        // Get all the files and directories within
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_DIR) treeAddFolder(&t, ent->d_name);
            else treeAddFile(&t, ent->d_name);
        }
        closedir(dir);
    } else {
        die("opendir");
    }
    return t;
}

void drawTree(tree *t, abuf *ab, int y) {
    int filerow = y + t->rowoff;
    if (filerow < t->numfolders) {
        // Render folders first
    } else {
        // Render files
    }
    
    // Extra pipe at the end
    abAppend(ab, " |", 2);
    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}
