#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

struct ls_dirent {
    struct dirent   dirent;
    struct stat     stat;
};

struct ls_options {
    bool    show_all;
    bool    long_listing;
    char *  path;
};

static void
get_date_string(int st_time, char *buf)
{
    char *lut[] = {
        "Jan", "Feb", "Mar", "Apr",
        "May", "Jun", "Jul", "Aug",
        "Sep", "Oct", "Nov", "Dec", "Unk"
    };

    time_t time = (time_t)st_time;

    struct tm *tm = gmtime(&time);
    
    sprintf(buf, "%s %2d %2d:%d", lut[tm->tm_mon], tm->tm_mday, tm->tm_hour, tm->tm_min);
}

static void
get_mode_string(int st_mode, char *buf)
{
    *(buf++) = (S_ISDIR(st_mode)) ? 'd' : '-';
    *(buf++) = (st_mode & S_IRUSR) ? 'r' : '-';
    *(buf++) = (st_mode & S_IWUSR) ? 'w' : '-';
    *(buf++) = (st_mode & S_IXUSR) ? 'x' : '-';
    *(buf++) = (st_mode & S_IRGRP) ? 'r' : '-';
    *(buf++) = (st_mode & S_IWGRP) ? 'w' : '-';
    *(buf++) = (st_mode & S_IXGRP) ? 'x' : '-';
    *(buf++) = (st_mode & S_IROTH) ? 'r' : '-';
    *(buf++) = (st_mode & S_IWOTH) ? 'w' : '-';
    *(buf++) = (st_mode & S_IXOTH) ? 'x' : '-';
    *(buf++) = 0;
}

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
            /*
            case DT_LNK:
                printf("\033[0;36m");
                break;
            */
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
ls_long_print(struct ls_dirent **entries, int nentries)
{
    for (int i = 0; i < nentries; i++) {
        struct ls_dirent *entry = entries[i];
        char mode_str[11];
        char date_str[15];

        get_date_string(entry->stat.st_mtime, date_str);
        get_mode_string(entry->stat.st_mode, mode_str);

        printf("%s ", mode_str);
        printf("%-4d ", entry->stat.st_uid);
        printf("%-4d ", entry->stat.st_gid);
        printf("%8d ", (int)entry->stat.st_size);
        printf("%s ", date_str);
        printf("%s\n", entry->dirent.d_name);

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
read_dirents(struct ls_options *options, struct ls_dirent **entries, int nentries)
{
    DIR *dirp = opendir(options->path);

    if (!dirp) {
        return -1;
    }

    struct dirent *dirent;

    int results = 0;

    for (int n = 0; n < nentries && (dirent = readdir(dirp)); n++) {
        if (dirent->d_name[0] != '.' || options->show_all) {
            struct ls_dirent *entry = (struct ls_dirent*)calloc(1, sizeof(struct ls_dirent));

            memcpy(&entry->dirent, dirent, sizeof(struct dirent));

            entries[results++] = entry;

            char buf[512];

            snprintf(buf, 512, "%s/%s", options->path, entry->dirent.d_name);

            if (stat(buf, &entry->stat)) {
                printf("call to stat() failed for file %s\n", buf);
            }
        }
    }

    closedir(dirp);

    return results;
}

static int
parse_arguments(struct ls_options *options, int argc, char *argv[])
{
    int c;

    while (optind < argc) {
        if ((c = getopt(argc, argv, "al")) != -1) {
            switch (c) {
                case 'l':
                    options->long_listing = true;
                    break;
                case 'a':
                    options->show_all = true;
                    break;
                case '?':
                    return -1;
            }
        } else {
            char *path = argv[optind++];

            options->path = path;
        }
    }
    
    if (options->path == NULL) {
        options->path = ".";
    }

    return 0;
}

int
main(int argc, char *argv[])
{
    struct ls_options options;

    memset(&options, 0, sizeof(struct ls_options));

    if (parse_arguments(&options, argc, argv)) {
        return -1;
    }

    struct ls_dirent *entries[512];

    int count = read_dirents(&options, entries, 512);

    sort_dirents_alphabetical(entries, count);

    if (options.long_listing) {
        ls_long_print(entries, count);
    } else {
        ls_pretty_print(entries, count);
    }

}
