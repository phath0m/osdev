#include <stdio.h>
#include <string.h>

const char *cow[] = {
    "\\   ^__^",
    " \\  (oo)\\_______",
    "    (__)\\       )\\/\\",
    "        ||----w |",
    "        ||     ||",
    NULL
};

static void
print_nchars(char c, int n)
{
    int i;
    char buf[80];

    for (i = 0; i < n && i < 80; i++) {
        buf[i] = c;
    }

    buf[i] = 0;

    fputs(buf, stdout);
}

int
main (int argc, char *argv[])
{
    char buf[512];

    if (argc <= 1) {
        fgets(buf, sizeof(buf), stdin);
    } else {
        buf[0] = 0;
        int i;
        for (i = 1; i < argc; i++) {
            strncat(buf, argv[i], 512);
            strncat(buf, " ", 1);
        }
    }

    size_t len = strlen(buf);

    if (buf[len - 1] == '\n') {
        buf[len -1] = '\0';
        len -= 1;
    }

    fputs("   ", stdout);
    print_nchars('-', len);
    fputs("\n", stdout);
    fprintf(stdout, " < %s >\n   ", buf);
    print_nchars('-', len);
    fputs("\n", stdout);

    int i;
    for (i = 0; cow[i]; i++) {
        print_nchars(' ', (len / 2) + 3);
        printf("%s\n", cow[i]);
    }

    return 0;
}

