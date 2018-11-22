#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ls_dirent {
    struct dirent dirent;
};

static void
ls_pretty_print(struct ls_dirent **contents, int nentries)
{
    int entries = 0;
    int longest = 0;

    for (int i = 0; i < nentries; i++) {
        struct ls_dirent *entry = contents[i];

        int len = strlen(entry->dirent.d_name);

        if (len > longest) {
            longest = len;
        }

        entries++;
    }

    int cols = 80 / (longest + 2);
    int rows = entries / cols;
    int padding = longest + 2;

    int row = 0;
    int col = 0;

    for (int i = 0; i < nentries; i++) {
        struct ls_dirent *entry = contents[i];

        int len = strlen(entry->dirent.d_name);

        switch (entry->dirent.d_type) {
            case DT_BLK:
                printf("\033[0;33m");
                break;
            case DT_CHR:
                printf("\033[0;33m");
                break;
            case DT_DIR:
                printf("\033[1;34m");
                break;
            case DT_LNK:
                printf("\033[0;36m");
                break;
        }

        printf("%s\033[0m", entry->dirent.d_name);

        if (col != cols) {
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

static void
sort_dirents_alphabetical(struct ls_dirent **entries, int nentries)
{
    for (int i = 0; i < nentries; i++) {
        for (int j = i + 1; j < nentries; j++) {
            struct ls_dirent *entry1 = entries[i];
            struct ls_dirent *entry2 = entries[j];

            if (strcmp(entry1->dirent.d_name, entry2->dirent.d_name) > 0) {
                entries[i] = entry2;
                entries[j] = entry1;
            }
        }
    }
}

static int
read_dirents(DIR *dirp, struct ls_dirent **entries, int nentries)
{
    struct dirent *dirent;

    int results = 0;

    for (int n = 0; n < nentries && (dirent = readdir(dirp)); n++) {

        if (dirent->d_name[0] != '.') {
            struct ls_dirent *entry = (struct ls_dirent*)calloc(1, sizeof(struct ls_dirent));

            memcpy(&entry->dirent, dirent, sizeof(struct dirent));

            entries[results++] = entry;
        }
    }

    return results;
}

static int
list_dir(const char *path)
{
    DIR *dirp = opendir(path);

    if (dirp) {
        struct ls_dirent *entries[512];

        int count = read_dirents(dirp, entries, 512);

        sort_dirents_alphabetical(entries, count);

        ls_pretty_print(entries, count);

        closedir(dirp);

        return 0;
    }

    return -1;
}

int
main(int argc, const char *argv[])
{
    DIR *dirp;

    if (argc > 1) {
        return list_dir(argv[1]);
    } else {
        return list_dir(".");
    }
}
