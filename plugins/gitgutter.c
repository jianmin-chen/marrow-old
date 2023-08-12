/*** parse the output of `git diff` ***/
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "../status/error.h"
#include "../libs/buffer.h"

void getdiff(char *dirname) {
    // Pipe system call to `git diff`.
    if (dirname != NULL) {}
    
    FILE *pptr;
    pptr = popen("git --no-pager diff", "r");
    
    if (pptr == NULL) {
        die("popen");
        return;
    }


    int ch;
    while ((ch=fgetc(pptr)) != EOF) {
        if (ch == '\n') {
            // Should be after newline?
            int hunk;
            ch = fgetc(pptr);
            hunk = ch == '@';
            if (!hunk) continue;
            ch = fgetc(pptr);
            hunk = ch == '@';
            if (hunk) {
                // Parse hunk
            }
        }
    }

    pclose(pptr);
}
