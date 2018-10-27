#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
print_usage()
{
    printf("usage: doit --start-directory <directory>\n");
}

static void
start_script(char *script)
{
    int pid = fork();

    if (pid == 0) {
        char *argv[2];
        argv[0] = script;
        argv[1] = NULL;
        execv(script, argv);
    }
}

static void
do_directory(const char *directory)
{
    struct dirent *dirent;

    DIR *dirp = opendir(directory);

    while ((dirent = readdir(dirp))) {
        if (strncmp(dirent->d_name, "start-", 6) == 0) {
            char full_path[256];
            sprintf(full_path, "%s/%s", directory, dirent->d_name);
            start_script(full_path);
        }
    }

    closedir(dirp);
}

int
main(int argc, const char *argv[])
{
    if (argc > 1) {
        const char *operation = argv[1];

        if (strcmp(operation, "--start-directory") == 0 && argc == 3) {
            do_directory(argv[2]);
        } else {
            print_usage();
        }
    } else {
        const char *console = getenv("CONSOLE");

        for (int i = 0; i < 3; i++) {
            open(console, O_RDWR);
        }

        printf("doit: Welcome!\n");
        do_directory("/etc/doit.d");
    }

    return 0;
}
