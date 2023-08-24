/*** parse the output of `git diff` ***/
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include "../status/error.h"
#include "../libs/buffer.h"

#define GITDIFF "git --no-pager diff "

int gitdiff(char *dirname, int *buf, size_t bufsize) {
    // Pipe system call to `git diff`.

    int commandsize = strlen(GITDIFF) + strlen(dirname) + 1;
    char *command = malloc(commandsize);
    snprintf(command, commandsize, "%s%s", GITDIFF, dirname);

    FILE *pptr;
    pptr = popen(command, "r");

    if (pptr == NULL) die("popen");

    fseek(pptr, 0, SEEK_END);
    long fsize = ftell(pptr);
    fseek(pptr, 0, SEEK_SET);

    char *s = malloc(fsize + 1);
    fread(s, fsize, 1, pptr);

    int i = 0;

    for (int j = 0; j < 4 && s[i] != EOF; j++) {
        // Skip first four lines
        while (s[i] != '\n') i++;
        i++;
    }

    size_t buflen = 0;

    // For every hunk, there are a couple of pieces:
    // @@ start, oldlines +start, newlines @@ code
    // Before the next hunk starts, or end of file:
    // Start incrementing value from oldlines
    // If line starts with "-", that means that oldline was removed there
    // If next line starts with "+", that means that the oldline was edited, skip since we're using tildes anyways
    // If line starts with "+", that means that the newline was added

    while (s[i] != NULL) {
        if (s[i] == '@' && s[i + 1] == '@' && s[i + 2] == ' ') {
            // Read hunk
            i += 4;

            // Read starting line
            int j = i;
            while (s[j] != ',') j++;
            char *start = malloc(sizeof(char) * (j - i));
            while (i < j) {
                char digit[2] = {s[i], '\0'};
                strcat(start, digit);
                i++;
            }
            i++;  // Now add the comma!

            // Now determine whether lines were added or deleted
            j = 

            j = i;
            while (s[j] != ',') j++;
            i = j + 1;
            j = i;
            while (s[j] != ' ') j++;
            char *end = malloc(sizeof(char) * (j - i));
            while (i < j) {
                char digit[2] = {s[i], '\0'};
                strcat(end, digit);
                i++;
            }

            int startline = atoi(start);
            int endline = atoi(end);

            while (s[i] != '\n') i++;
            i++;

            // Keep reading until next hunk
            for (int line = startline; line < startline + endline; line++) {
                if (buflen == bufsize) {
                    // Resize buffer of changed lines if necessary
                    bufsize *= 2;
                    buf = realloc(buf, bufsize);
                }

                if (s[i] == '-') {
                    buf[buflen] = line;
                    buflen++;
                    while (s[i] != '\n') i++;
                    i++;
                    if (s[i] == '+') {
                        // Line was edited
                        while(s[i] != '\n') i++;
                        i++;
                    }
                } else if (s[i] == '+') {
                    // New line added
                    buf[buflen] = line;
                    buflen++;
                    while (s[i] != '\n') i++;
                    i++;
                } else {
                    // Read line as usual
                    while (s[i] != '\n') i++;
                    i++;
                }
            }
        }
    }

    free(command);
    free(s);
    pclose(pptr);

    return buflen;
}
