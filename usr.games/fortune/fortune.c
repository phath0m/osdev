#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

int
main(int argc, char *argv[])
{
    FILE *fp = fopen("/usr/share/games/fortunes.txt", "r");

    if (!fp) {
        perror("fortune");
        return -1;
    }

    int linecount = 0;
    char line[1024];

    while (fgets(line, 1023, fp)) {
        linecount++;
    }

    srand(time(NULL));
    fseek(fp, 0, SEEK_SET);

    int index = rand() % linecount;
    int i = 0;

    while (fgets(line, 1023, fp)) {
        if (i == index) {
            fputs(line, stdout);
            break;
        }
        i++;
    }

    fclose(fp);

    return 0;
}

