#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

static int
settime(const char *timestr)
{
    int res;
    size_t len;
    time_t now;
    struct timeval tv;
    struct tm tm;

    now = time(NULL);

    localtime_r(&now, &tm);

    len = strlen(timestr);

    res = -1;

    switch (len) {
        case 4: /* HHmm */
            res = sscanf(timestr, "%02d%02d", &tm.tm_hour, &tm.tm_min);
            break;
        case 8: /* YYYYMMDD */
            res = sscanf(timestr, "%04d%02d%02d",
                    &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
            tm.tm_year -= 1900; /* struct tm is stupid and stores the year relative to 1900 */
            tm.tm_mon -= 1; /* month is zero based */
            break;
        case 12: /* YYYYMMDDHHmm*/       
            res = sscanf(timestr, "%04d%02d%02d%02d%02d",
                    &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour,
                    &tm.tm_min);
            tm.tm_year -= 1900; /* struct tm is stupid and stores the year relative to 1900 */
            tm.tm_mon -= 1; /* month is zero based */
            break;
    }

    if (res <= 0) {
        fprintf(stderr, "date: invalid date %s\n", timestr); 
        return -1;
    }

    tv.tv_sec = mktime(&tm);
    tv.tv_usec = 0;

    if (settimeofday(&tv, NULL) != 0) {
        perror("date");
        return -1;
    }

    return 0;
}

int
main(int argc, char *argv[])
{
    time_t now;
    struct tm *nowp;

    now = time(NULL);

    if (argc == 1) {
        nowp = gmtime(&now);
        fputs(asctime(nowp), stdout);
        return 0;
    }
   
    settime(argv[1]);

    return 0;
}