#include <dirent.h>
#include <stdio.h>
#include <string.h>

void tree(char *path) {
	DIR *d = opendir(path);
	if (d == NULL) {
		exit(2);
	}
}
