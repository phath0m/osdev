#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

ssize_t
getline(char **lineptr, size_t *n, FILE *stream)
{
    size_t pos;
    int c;

    if (lineptr == NULL || stream == NULL || n == NULL) {
        errno = EINVAL;
        return -1;
    }

    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }

    if (*lineptr == NULL) {
        *lineptr = malloc(128);
        if (*lineptr == NULL) {
            return -1;
        }
        *n = 128;
    }

    pos = 0;
    while(c != EOF) {
        if (pos + 1 >= *n) {
            size_t new_size = *n + (*n >> 2);
            if (new_size < 128) {
                new_size = 128;
            }
            char *new_ptr = realloc(*lineptr, new_size);
            if (new_ptr == NULL) {
                return -1;
            }
            *n = new_size;
            *lineptr = new_ptr;
        }

        ((unsigned char *)(*lineptr))[pos ++] = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }

    (*lineptr)[pos] = '\0';
    return pos;
}

static void
do_grep(const char *pattern, FILE *fp)
{
    size_t len = 0;
    ssize_t read;

    char *line = NULL;

    while ((read = getline(&line, &len, fp)) != -1) {
        if (strstr(line, pattern) != NULL) {
            fputs(line, stdout);
        }               
    }    
}

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: grep <pattern> [file]\n");
        return -1;
    } 

    if (argc == 2) {
        do_grep(argv[1], stdin); 
    } else if (argc == 3) {
        FILE *fp = fopen(argv[2], "r");

        if (!fp) {
            perror("grep");
            return -1;
        }

        do_grep(argv[1], fp);

        fclose(fp);
    }
    return 0;
}

