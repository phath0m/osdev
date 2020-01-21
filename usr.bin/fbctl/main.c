#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

struct fbctl_options {
    const char *    background;
    int             alpha;
};

struct lfb_req {
    uint8_t     opcode;
    uint32_t    length;
    uint32_t    offset;
    uint32_t    color;
    void *      data;
};

static int
parse_arguments(struct fbctl_options *options, int argc, char *argv[])
{
    int c;

    while (optind < argc) {
        if ((c = getopt(argc, argv, "a:b:")) != -1) {
            switch (c) {
                case 'b':
                    options->background = optarg;
                    break;
                case 'a':
                    options->alpha = atoi(optarg);
                    break;
                case '?':
                    return -1;
            }
        }
    }

    return 0;
}

static int
change_background(struct fbctl_options *options)
{
    int fd = open("/dev/lfb", O_WRONLY);

    if (fd < 0) {
        perror("fbctl");
        return -1;
    }

    int bg = open(options->background, O_RDONLY);

    if (bg < 0) {
        perror("fbctl");
        return -1;
    }

    void *buf = calloc(1, 65536);

    for (int i = 0; i < 1920000; i += 65536) {
        int nbytes = read(bg, buf, 65536);
        
        /* write to background buffer */
        struct lfb_req req;
        req.opcode = 0;
        req.length = nbytes;
        req.offset = i;
        req.data = buf;

        ioctl(fd, 0x200, &req);
    }

    close(bg);

    /* draw background and shade with provided alpha value */
    struct lfb_req req;

    req.opcode = 1;
    req.color = options->alpha;

    ioctl(fd, 0x200, &req);

    free(buf);

    close(fd);


    return 0;
}

int
main(int argc, char *argv[])
{
    struct fbctl_options options;

    options.alpha = 255;

    if (parse_arguments(&options, argc, argv) != 0) {
        return -1;
    }

    if (options.background) {
        change_background(&options);
    }

    return 0;
}


