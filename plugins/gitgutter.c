/*** parse the output of `git diff` ***/
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "../status/error.h"
#include "../libs/buffer.h"

void gitdiff(char *dirname) {
    // Pipe system call to `git diff`.
    char *command = "git --no-pager diff";
    if (dirname != NULL) {
        // Adjust command for specific file
        snprintf(command, sizeof(command) + sizeof(dirname), "%s%s", command, dirname);
        printf("%s", command);
        exit(1);
    }
    
    FILE *pptr;
    pptr = popen(command, "r");
    
    if (pptr == NULL) die("popen");

    fseek(pptr, 0, SEEK_END);
    long fsize = ftell(pptr);
    fseek(pptr, 0, SEEK_SET);

    char *s = malloc(fsize + 1);
    fread(s, fsize, 1, pptr);

    for (int i = 0; i < fsize; i++) {
        if (s[i] != '@' || s[i - 1] != '\n') continue;

    }

    free(s);
    pclose(pptr);
}
