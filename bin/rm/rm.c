#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

struct rm_options {
    bool    force;
    bool    recursive;
};

static bool
prompt_to_delete(const char *path)
{
    char ch;

    fprintf(stderr, "rm: remove regular file '%s'? ", path);
    scanf(" %c", &ch);

    return ch == 'y' || ch == 'Y';
}

static int remove_directory(struct rm_options *options, const char *path);

static int
remove_file(struct rm_options *options, const char *path)
{
    struct stat buf;

    if (stat(path, &buf) != 0) {
        perror("rm");
        return -1;
    }

    if (S_ISDIR(buf.st_mode)) {
        return remove_directory(options, path);
    }

    if (S_ISREG(buf.st_mode)) {
        
        if (!options->force && !prompt_to_delete(path)) {
            return 0;
        }

        if (unlink(path) != 0) {
            perror("rm");
            return -1;
        }
    }

    return 0;
}

static int
remove_directory(struct rm_options *options, const char *dirpath)
{
    DIR *dp;

    char childpath[512];

    struct dirent *dirent;

    dp = opendir(dirpath);

    if (!dp) {
        perror("rm");
        return -1;
    }

    while ((dirent = readdir(dp))) {
        if (strcmp(dirent->d_name, "..") == 0 || strcmp(dirent->d_name, ".") == 0) {
            continue;
        }

        sprintf(childpath, "%s/%s", dirpath, dirent->d_name);

        if (remove_file(options, childpath) != 0) {
            closedir(dp);
            return -1;
        }
    }

    closedir(dp);
    
    if (rmdir(dirpath) != 0) {
        perror("rm");
        return -1;
    }

    return 0;
}

int
main(int argc, char *argv[])
{
    int c;
    int i;

    struct rm_options options;

    while (optind < argc) {
        if ((c = getopt(argc, argv, "rf")) != -1) {
            switch (c) {
                case 'f':
                    options.force = true;
                    break;
                case 'r':
                    options.recursive = true;
                    break;
                case '?':
                    return -1;
            }
        } else {
            break;
        }
    }

    for (i = optind; i < argc; i++) {
        remove_file(&options, argv[i]);
    }

    return 0;
}
