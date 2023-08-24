#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include "../status/error.h"

/*** defines ***/

#define SHELL "/bin/marrow"

typedef struct pty {
    int master;
    int slave;
} pty;
typedef struct emulator {
}

void pt_pair(struct pty *p) {
    char *slave_name;

    p->master = posix_openpt(O_RDWR | O_NOCTTY);
    if (p->master == -1) die("posix_openpt");

    /* grantpt() and unlockpt() are housekeeping functions that have to
     * be called before we can open the slave FD. Refer to the manpages
     * on what they do.
     */
    if (grantpt(p->master) == -1) die("grantpt");
    if (unlockpt(p->master) == -1) die("unlockpt");

    /* 
     * Up until now, we only have the master FD. We also need a file
     * descriptor for our child process. We get it by asking for the
     * actual path in /dev/pts which we then open using a regular
     * open(). So, unlike pipe(), you don't get two corresponding file
     * descriptors in one go.
     */

    slave_name = ptsname(p->master);
    if (slave_name == NULL) die("ptsname");

    p->slave = open(slave_name, O_RDWR | O_NOCTTY);
    if (p->slave == -1) die("open(slave_name)");
}

void spawn(pty *p) {
    pid_t pid;
    char *env[] = {"TERM=marrow", NULL};

    pid = fork();
    if (pid == 0) {
        close(p->master);

        /*
         * Create a new session ane make our terminal this process'
         * controlling terminal. The shell that we'll spawn in a second
         * will inherit the status of session leader.
         */
        setsid();
        if (ioctl(p->slave, TIOCSCTTY, NULL) == -1) die("ioctl(TIOCSCTTY)");

        dup2(p->slave, 0);
        dup2(p->slave, 1);
        dup2(p->slave, 2);
        close(p->slave);

        execle(SHELL, "-" SHELL, (char *) NULL, env);
    } else if (pid > 0) close(p->slave);
    die("fork");
}