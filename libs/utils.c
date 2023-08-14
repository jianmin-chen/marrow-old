#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "../status/error.h"

char *run(char *command) {
    FILE *pptr;
    pptr = popen(command, "r");

    if (pptr == NULL) die("popen");

    fseek(pptr, 0, SEEK_END);
    long fsize = ftell(pptr);
    fseek(pptr, 0, SEEK_SET);

    char *res = malloc(fsize + 1);
    fread(res, fsize, 1, pptr);

    pclose(pptr);
    return res;
}