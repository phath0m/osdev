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

struct targa_hdr {
    uint8_t     id_len;
    uint8_t     map_type;
    uint8_t     img_type;
    uint16_t    map_first;
    uint16_t    map_len;
    uint8_t     map_entry_size;
    uint16_t    x;
    uint16_t    y;
    uint16_t    width;
    uint16_t    height;
    uint8_t     bpp;
    uint8_t     misc;
} __attribute__((packed));


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

void *
open_tga_image(const char *image_path)
{
    FILE *fp = fopen(image_path, "r");

    if (!fp) {
        return NULL;
    }

    struct targa_hdr header;

    fread(&header, 1, sizeof(header), fp);

    if (header.width != 600 || header.height != 800) {
        return NULL;
    }

    int bpp = header.bpp / 8;

    uint32_t *buf = calloc(1, header.width*header.height*sizeof(uint32_t));

    for (int y = header.height - 1; y >= 0; y--)
    for (int x = 0; x < header.width; x++) {
        uint8_t rgb[4];

        fread(rgb, 1, bpp, fp);

        buf[y * header.width + x] = rgb[0] | (rgb[1] << 8) | (rgb[2] << 16);
    }

    fclose(fp);

    return buf;
}

static int
change_background(struct fbctl_options *options)
{
    int fd = open("/dev/lfb", O_WRONLY);

    if (fd < 0) {
        perror("fbctl");
        return -1;
    }


    void *buf = open_tga_image(options->background);
 
    if (!buf) {
        printf("fbctl: could not load image\n");
        return NULL;
    }

    /* write to background buffer */
    struct lfb_req req;
    req.opcode = 0;
    req.length = 1920000;
    req.offset = 0;
    req.data = buf;

    ioctl(fd, 0x200, &req);

    /* draw background and shade with provided alpha value */
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


