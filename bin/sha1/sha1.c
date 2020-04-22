#include <stdio.h>
#include <stdint.h>
#include "sha1.h"

static void
usage()
{
    fputs("usage: sha1 [file]\n", stdout);
}

int
main(int argc, char *argv[])
{
    if (argc != 2) {
        usage();
        return -1;
    }

    FILE *fp = fopen(argv[1], "r");

    if (!fp) {
        perror("sha1");
        return -1;
    }

    uint8_t buf[1024];
    uint8_t digest[20];
    ssize_t nread = 0;

    SHA1_CTX ctx;
    SHA1Init(&ctx);

    while ((nread = fread(buf, 1, 1024, fp)) > 0) {
        SHA1Update(&ctx, buf, nread);
    }

    SHA1Final(digest, &ctx);

    for (int i = 0; i < 20; i++) {
        printf("%02x", digest[i]);
    }

    printf(" %s \n", argv[1]);

    return 0;
}
