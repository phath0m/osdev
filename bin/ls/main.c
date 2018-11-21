#include <dirent.h>
#include <stdio.h>
#include <string.h>

static void
ls_pretty_print(DIR *dirp)
{
    struct dirent *dirent;

    int entries = 0;
    int longest = 0;

    while (dirent = readdir(dirp)) {
        int len = strlen(dirent->d_name);

        if (len > longest) {
            longest = len;
        }

        entries++;
    }

    rewinddir(dirp);

    int cols = 80 / (longest + 2);
    int rows = entries / cols;
    int padding = longest + 2;

    int row = 0;
    int col = 0;
    printf("We can have %d cols\n", cols);
    while (dirent = readdir(dirp)) {
        int len = strlen(dirent->d_name);

        switch (dirent->d_type) {
            case DT_BLK:
                printf("\033[0;36m");
                break;
            case DT_CHR:
                printf("\033[0;37m");
                break;
            case DT_DIR:
                printf("\033[0;32m");
                break;
        }

        printf("%s\033[0m", dirent->d_name);

        if (rows <= 1) {
            printf(" ");
        } else {
            int spaces = padding - len;

            for (int i = 0; i < spaces; i++) {
                printf(" ");
            }
        }

        col++;

        if (col > cols) {
            col = 0;
            row++;
            printf("\n");
        }
    }

    if (col > 0) {
        printf("\n");
    }
}

int
main(int argc, const char *argv[])
{
    DIR *dirp;

    if (argc > 1) {
        dirp = opendir(argv[1]);
    } else {
        char cwd[512];
        getcwd(cwd, 612);
        dirp = opendir(cwd);
    }

    if (dirp) {
        ls_pretty_print(dirp);
        return 0;
    }
    return -1;
}
