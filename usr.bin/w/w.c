#include <stdio.h>
#include <time.h>
#include <utmp.h>

static void
get_time_string(char *buf, size_t buf_size, time_t stime)
{
    time_t now = time(NULL);

    struct tm *tp = gmtime(&stime);

    if (now - stime < 86400) {
        strftime(buf, buf_size, "%R", tp);
    } else {
        strftime(buf, buf_size, "%b%d", tp);
    }
}

int
main(int argc, char *argv[])
{
    if (argc != 1) {
        fprintf(stderr, "usage: w\n");
        return -1;
    }

    FILE *fp = fopen("/var/run/utmp", "r");

    if (!fp) {
        perror("w");
        return -1;
    }

    puts("USER     TTY      FROM             LOGIN@");

    struct utmp utmp;
    char login_time[6];

    while (fread(&utmp, 1, sizeof(utmp), fp) > 0) {
        
        if (!utmp.ut_name[0]) {
            continue;
        }

        printf("%-9s", utmp.ut_name);
        printf("%-9s", utmp.ut_line);
        printf("%-17s", "-");

        get_time_string(login_time, 6, utmp.ut_time);

        printf("%s\n", login_time); 
    }

    return 0;
}

