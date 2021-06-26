#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#define LS_INC_HIDDEN   0x01
#define LS_LONG_FMT     0x02
#define LS_REVERSE      0x04

typedef enum {
    SORT_ALPHA,
    SORT_MTIME
} sort_mode_t;

struct ls_dirent {
    struct dirent   dirent;
    struct stat     stat;
};

struct ls_options {
    int             flags;
    sort_mode_t     sort_mode;
    char *          path;
};

static void
get_date_string(int st_time, char *buf, size_t buf_size)
{
    static struct tm now_buf;
    static struct tm *tm_now = NULL;

    time_t now;
    time_t stat_time;

    struct tm *tm_then;
    
    if (!tm_now) {
        now = time(NULL);
        tm_now = gmtime_r(&now, &now_buf);
    }

    stat_time = (time_t)st_time;
    tm_then = gmtime(&stat_time);

    if (tm_then->tm_year == tm_now->tm_year) {
        strftime(buf, buf_size, "%b %e %H:%M", tm_then);
    } else {
        strftime(buf, buf_size, "%b %e  %Y", tm_then);
    }
}

static void
get_mode_string(struct ls_dirent *ent, char *buf)
{
    int st_mode;
    
    st_mode = ent->stat.st_mode;

    switch (ent->dirent.d_type) {
        case DT_DIR:
            *(buf++) = 'd';
            break;
        case DT_CHR:
            *(buf++) = 'c';
            break;
        case DT_FIFO:
            *(buf++) = 'p';
            break;
        case DT_SOCK:
            *(buf++) = 's';
            break;
        default:
            *(buf++) = '-';
            break;
    }

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
ls_print_color(struct ls_dirent *entry)
{
    switch (entry->dirent.d_type) {
        case DT_BLK:
            printf("\033[0;33m");
            return;
        case DT_CHR:
            printf("\033[0;33m");
            return;
        case DT_DIR:
            printf("\033[0;34m");
            return;
        case DT_FIFO:
            printf("\033[0;36m");
            return;
        /*
        case DT_LNK:
            printf("\033[0;36m");
            return;
        */
    }

    if ((entry->stat.st_mode & S_IXOTH) ||
        (entry->stat.st_mode & S_IXGRP) ||
        (entry->stat.st_mode & S_IXUSR))
    {
        printf("\033[0;32m");
    }
}

static void
ls_pretty_print(struct ls_dirent **contents, int nentries)
{
    int col;
    int cols;
    int entries;
    int i;
    int longest;
    int padding;
    int row;

    struct winsize ws;

    entries = 0;
    longest = 0;

    for (i = 0; i < nentries; i++) {
        int len;
        struct ls_dirent *entry;
        
        len = strlen(entry->dirent.d_name);
        entry = contents[i];

        if (len > longest) {
            longest = len;
        }
        entries++;
    }

    ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);

    cols = (ws.ws_col - longest )  / (longest + 2);
    padding = longest + 2;
    row = 0;
    col = 0;

    for (i = 0; i < nentries; i++) {
        int len;
        struct ls_dirent *entry;

        entry = contents[i];
        len = strlen(entry->dirent.d_name);

        ls_print_color(entry);
        printf("%s\033[0m", entry->dirent.d_name);

        if (col != cols) {
            int j;
            int spaces;
            
            spaces = padding - len;
            for (j = 0; j < spaces; j++) {
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
    int i;

    char mode_str[11];
    char date_str[15];

    struct group *grp;
    struct ls_dirent *entry;
    struct passwd *pwd;

    for (i = 0; i < nentries; i++) {
        entry = entries[i];

        get_date_string(entry->stat.st_mtime, date_str, 15);
        get_mode_string(entry, mode_str);

        pwd = getpwuid(entry->stat.st_uid);
        grp = getgrgid(entry->stat.st_gid);

        printf("%s ", mode_str);
        
        if (pwd) {
            printf("%-8s ", pwd->pw_name);
        } else {
            printf("%-8d ", entry->stat.st_uid);
        }

        if (grp) {
            printf("%-8s ", grp->gr_name);
        } else {
            printf("%-8d ", entry->stat.st_gid);
        }
        
        printf("%8d ", (int)entry->stat.st_size);
        printf("%s ", date_str);
        ls_print_color(entry);
        printf("%s\033[0m\n", entry->dirent.d_name);

    }
}

static void
sort_dirents_alphabetical(struct ls_dirent **entries, int nentries)
{
    int i;
    int j;

    struct ls_dirent *entry1;
    struct ls_dirent *entry2;

    for (i = 0; i < nentries; i++) {
        for (j = i + 1; j < nentries; j++) {
            entry1 = entries[i];
            entry2 = entries[j];

            if (strcmp(entry1->dirent.d_name, entry2->dirent.d_name) > 0) {
                entries[i] = entry2;
                entries[j] = entry1;
            }
        }
    }
}

static void
sort_dirents_modified_time(struct ls_dirent **entries, int nentries)
{
    int i;
    int j;

    struct ls_dirent *entry1;
    struct ls_dirent *entry2;

    for (i = 0; i < nentries; i++) {
        for (j = i + 1; j < nentries; j++) {
            entry1 = entries[i];
            entry2 = entries[j];

            if (entry1->stat.st_mtime > entry2->stat.st_mtime) {
                entries[i] = entry2;
                entries[j] = entry1;
            }
        }
    }
}

static void
reverse_dirents(struct ls_dirent **entries, int nentries)
{
    int i;

    struct ls_dirent *entry1;
    struct ls_dirent *entry2;

    for (i = 1; i <= nentries; i++) {
        entry1 = entries[nentries - i];
        entry2 = entries[i - 1];

        entries[nentries - i] = entry2;
        entries[i - 1] = entry1;
    }
}

static int
read_dirents(struct ls_options *options, struct ls_dirent **entries, int nentries)
{
    bool show_all;

    int n;
    int results;

    DIR *dirp;

    char buf[512];

    struct dirent *dirent;
    
    dirp = opendir(options->path);

    if (!dirp) {
        return -1;
    }

    results = 0;
    show_all = (options->flags & LS_INC_HIDDEN);

    for (n = 0; n < nentries && (dirent = readdir(dirp)); n++) {
        struct ls_dirent *entry;

        if (dirent->d_name[0] != '.' || show_all) {
            entry = calloc(1, sizeof(struct ls_dirent));
            memcpy(&entry->dirent, dirent, sizeof(struct dirent));
            entries[results++] = entry;

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
    char *path;

    while (optind < argc) {
        if ((c = getopt(argc, argv, "alrt")) != -1) {
            switch (c) {
                case 'l':
                    options->flags |= LS_LONG_FMT;
                    break;
                case 'a':
                    options->flags |= LS_INC_HIDDEN;
                    break;
                case 'r':
                    options->flags |= LS_REVERSE;
                    break;
                case 't':
                    options->sort_mode = SORT_MTIME;
                    break;
                case '?':
                    return -1;
            }
        } else {
            path = argv[optind++];
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
    int count;

    struct ls_options options;
    struct ls_dirent *entries[512];

    memset(&options, 0, sizeof(struct ls_options));

    if (parse_arguments(&options, argc, argv)) {
        return -1;
    }

    count = read_dirents(&options, entries, 512);

    if (count < 0) {
        perror("ls");
        return -1;
    }

    switch (options.sort_mode) {
        case SORT_ALPHA:
            sort_dirents_alphabetical(entries, count);
            break;
        case SORT_MTIME:
            sort_dirents_modified_time(entries, count);
            break;
    }

    if ((options.flags & LS_REVERSE)) {
        reverse_dirents(entries, count);
    }
    
    if ((options.flags & LS_LONG_FMT)) {
        ls_long_print(entries, count);
    } else {
        ls_pretty_print(entries, count);
    }
    
    return 0;
}
