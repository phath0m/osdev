#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

struct cp_options {
    bool    force;
    bool    recursive;
};

static bool
prompt_to_overwrite(const char *path)
{
    char ch;

    fprintf(stderr, "cp: overwrite existing file '%s'? ", path);
    scanf(" %c", &ch);

    return ch == 'y' || ch == 'Y';
}

static int
copy_file(struct cp_options *options, char *src, char *dst)
{
    ssize_t nread;
    FILE *d_fp;
    FILE *s_fp;

    char buf[1024];

    struct stat stat_buf;

    if (stat(src, &stat_buf) != 0) {
        perror("cp");
        return -1;
    }

    if (stat(dst, &stat_buf) == 0 && !options->force) {
        if (!prompt_to_overwrite(dst)) return 0;
    }

    s_fp = fopen(src, "r");

    if (!s_fp) {
        perror("cp");
        return -1;
    }

    d_fp = fopen(dst, "w");

    if (!d_fp) {
        perror("cp");
        return -1;
    }

    while ((nread = fread(buf, 1, 1024, s_fp)) > 0) {
        fwrite(buf, 1, nread, d_fp);
    }

    fclose(d_fp);
    fclose(s_fp);

    return 0;
}

int
main(int argc, char *argv[])
{
    int c;
    char *dst;
    char *src;

    struct cp_options options;
    
    memset(&options, 0, sizeof(options));

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

    if (argc - optind != 2) {
        return -1;
    } 
 
    src = argv[optind];
    dst = argv[optind + 1];   

    copy_file(&options, src, dst);

    return 0;
}
