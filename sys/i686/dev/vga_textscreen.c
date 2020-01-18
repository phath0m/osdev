/*
 * Basic driver for interacting with textscreen
 */

#include <string.h>
#include <sys/device.h>
#include <sys/types.h>
#include <sys/dev/textscreen.h>
#include <sys/i686/portio.h>

#define TEXTSCREEN_ADDR         0xC00B8000
#define TEXTSCREEN_HEIGHT       25
#define TEXTSCREEN_WIDTH        80
#define TEXTSCREEN_BUFFER_SIZE  TEXTSCREEN_HEIGHT*TEXTSCREEN_WIDTH

struct textscreen_state {
    uint8_t     foreground_color;
    uint8_t     background_color;
    uint16_t    position;
};

struct textscreen_state state;

static int textscreen_close(struct device *dev);
static int textscreen_ioctl(struct device *dev, uint64_t request, uintptr_t argp);
static int textscreen_open(struct device *dev);
static int textscreen_write(struct device *dev, const char *buf, size_t nbyte, uint64_t pos);

struct device textscreen_device = {
    .name   =   "vga",
    .mode   =   0600,
    .close  =   textscreen_close,
    .ioctl  =   textscreen_ioctl,
    .open   =   textscreen_open,
    .isatty =   NULL,
    .read   =   NULL,
    .write  =   textscreen_write,
    .state  =   &state
};

static void
update_cursor(uint16_t pos)
{
    io_write_byte(0x3D4, 0x0F);
    io_write_byte(0x3D5, (uint8_t) (pos & 0xFF));
    io_write_byte(0x3D4, 0x0E);
    io_write_byte(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

static void
clear_screen(uint8_t attr)
{
    uint16_t *textscreen_buffer = (uint16_t*)TEXTSCREEN_ADDR;
    uint16_t cell_value = ' ' | (attr << 8);

    for (int y = 0; y < TEXTSCREEN_HEIGHT; y++) {
        for (int x = 0; x < TEXTSCREEN_WIDTH; x++) {
            int pos = y * TEXTSCREEN_WIDTH + x;
            textscreen_buffer[pos] = cell_value;
        }
    }

}

static int
textscreen_close(struct device *dev)
{
    return 0;
}

static int
textscreen_ioctl(struct device *dev, uint64_t request, uintptr_t argp)
{
    struct textscreen_state *statep = (struct textscreen_state*)dev->state;

    switch (request) {
    case TEXTSCREEN_CLEAR:
        clear_screen((statep->background_color << 4) | statep->foreground_color);
        break;
    case TEXTSCREEN_SETBG:
        statep->background_color = (uint8_t)argp;
        break;
    case TEXTSCREEN_SETFG:
        statep->foreground_color = (uint8_t)argp;
        break;
    }

    return 0;
}

static int
textscreen_open(struct device *dev)
{
    return 0;
}

static inline void
textscreen_scroll(uint8_t attr)
{
    uint16_t *textscreen_buffer = (uint16_t*)TEXTSCREEN_ADDR;

    for (int y = 1; y < TEXTSCREEN_HEIGHT; y++) {
        for (int x = 0; x < TEXTSCREEN_WIDTH; x++) {
            int pos = y * TEXTSCREEN_WIDTH + x;
            textscreen_buffer[pos - TEXTSCREEN_WIDTH] = textscreen_buffer[pos];
        }
    }
    
    for (int x = 0; x < TEXTSCREEN_WIDTH; x++) {
        int y = TEXTSCREEN_WIDTH * (TEXTSCREEN_HEIGHT - 1);

        textscreen_buffer[y + x] = ' ' | (attr << 8);
    }
}

static int
textscreen_write(struct device *dev, const char *buf, size_t nbyte, uint64_t pos)
{
    struct textscreen_state *statep = (struct textscreen_state*)dev->state;

    uint16_t *textscreen_buffer = (uint16_t*)TEXTSCREEN_ADDR;

    for (int i = 0; i < nbyte; i++) {
        uint8_t attr = (statep->background_color << 4) | statep->foreground_color;

        char ch = buf[i];

        switch (ch) {
        case '\n':
            statep->position = (statep->position + 80) - (statep->position + 80) % 80;
            break;
        default:
            textscreen_buffer[statep->position++] = ch | (attr << 8);
            break;
        }
        
        if (statep->position >= TEXTSCREEN_BUFFER_SIZE) {
            textscreen_scroll(attr);
            statep->position = TEXTSCREEN_BUFFER_SIZE - TEXTSCREEN_WIDTH;
        }
    }

    update_cursor(statep->position);

    return nbyte;
}

__attribute__((constructor))
void
_init_vga()
{
    device_register(&textscreen_device);
    state.position = 0;
    state.foreground_color = 15;
    state.background_color = 0;
}
