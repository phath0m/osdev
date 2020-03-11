#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static bool
search_for_binary(char *name, char *resolved)
{
    if (!strncmp(name, "/", 1) || !strncmp(name, "./", 2)) {
        strcpy(resolved, name);
        return true;
    }

    char *path = getenv("PATH");

    if (!path) {
        return false;
    }

    static char path_copy[1024];

    strncpy(path_copy, path, 1024);

    char *searchdir = strtok(path_copy, ":");

    while (searchdir != NULL) {
        bool succ = false;

        DIR *dirp = opendir(searchdir);

        if (!dirp) {
            searchdir = strtok(NULL, ":");
            continue;
        }

        struct dirent *dirent;

        while ((dirent = readdir(dirp))) {
            if (strcmp(dirent->d_name, name) == 0) {
                sprintf(resolved, "%s/%s", searchdir, name);
                succ = true;
                break;
            }
        }

        closedir(dirp);

        if (succ) {
            return true;
        }

        searchdir = strtok(NULL, ":");
    }

    return false;
}

int
main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: which <name>\n");
        return -1;
    }

    char *path = getenv("PATH");
    
    if (!path) {
        return -1;
    }

    char resolved[1024];

    if (!search_for_binary(argv[1], resolved)) {
        fprintf(stderr, "which: no %s in (%s)\n", argv[1], path);
        return -1;
    }

    puts(resolved);

    return 0;
}

