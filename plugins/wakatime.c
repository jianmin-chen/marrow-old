#include "../status/error.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include "../libs/utils.h"
#include <ctype.h>

/*
    Taken from https://wakatime.com/help/creating-plugin.

    This is a high-level overview of a WakaTime plugin from the time it's
   loaded, until the editor is exited.

    * Plugin loaded by text editor/IDE, runs plugin's initialization code
    * Initialization code
        * Setup any global variables, like plugin version, editor/IDE version 
        * Check for wakatime-cli, or download into `~/.wakatime/` if missing or needs an update
        * Check for API key in `~/.wakatime.cfg`. Prompt user to enter if does not exist
        * Setup event listeners to detect when current file changes, a file is modified, and a file is saved
    * Current file changed
        * Go to `Send heartbeat` function with isWrite false
    * User types in a file (our file modified event listener code is run) 
        * Go to `Send heartbeat` function with isWrite false
    * A file is saved (our file save event listener code is run)
        * Go to `Send heartbeat` function with isWrite true
    * `Send heartbeat` function
        * Check `lastHeartbeat` variable. If isWrite is false, and file has not changed since last heartbeat, and less than 2 minutes since last heartbeat, then return and do nothing
        * Run wakatime-cli in background process passing it the current file
        * Update `lastHeartbeat` variable with current file and current time    
 */

void sendHeartbeat() {
    // wakatime-cli --entity myfile.txt --key waka_b2934042-e8a4-40e8-bc4d-6d9a7e4871dd
    
}

int wakatimeInstalled() { 
    DIR *dir = opendir("~/.wakatime");

    char *os = run("uname");
    char *architecture = run("uname -m");

    if (ENOENT == errno) {
        // Install wakatime-cli
        printf("Wakatime not found");
        #if defined(__linux__)
        #elif defined(__APPLE__) && defined(__MACH__)
        #elif defined(__unix__) && defined(BSD)
        #endif
        exit(1);
    }

    free(os);
    free(architecture);
    closedir(dir);
}