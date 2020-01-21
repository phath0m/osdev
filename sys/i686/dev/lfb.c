/*
 * Linear framebuffer driver for VBE
 * NOTICE: this does not enable VBE. This requires VBE to be initialized
 * We're letting GRUB do that because I'm lazy and don't feel like reading
 * a textwall on osdev wiki
 */

#include <stdlib.h>
#include <string.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/types.h>
#include <sys/dev/textscreen.h>
#include <sys/i686/multiboot.h>
// remove me
#include <stdio.h>

#include "./lfb_font.h"

struct lfb_state {
    vbe_info_t *    vbe;
    spinlock_t      lock;
    uint8_t *       frame_buffer;
    uint8_t *       foreground;
    uint8_t *       backbuffer;
    uint8_t *       background;
    int             width;
    int             height;
    int             pitch;
    int             depth;
    int             buffer_size;
    int             textscreen_width;
    int             textscreen_height;
    int             position;
    int             background_color;
    int             foreground_color;
    int *           color_palette;
};

static int default_color_palette[] = {0x353540, 0x8c5760, 0x7b8c58, 0x8c6e43, 0x58698c, 0x7b5e7d, 0x66808c, 0x8c8b8b};

static int lfb_close(struct device *dev);
static int lfb_ioctl(struct device *dev, uint64_t request, uintptr_t argp);
static int lfb_isatty(struct device *dev);
static int lfb_open(struct device *dev);
static int lfb_write(struct device *dev, const char *buf, size_t nbyte, uint64_t pos);

struct device lfb_device = {
    .name   =   "lfb",
    .mode   =   0600,
    .close  =   lfb_close,
    .ioctl  =   lfb_ioctl,
    .isatty =   lfb_isatty,
    .open   =   lfb_open,
    .read   =   NULL,
    .write  =   lfb_write,
    .state  =   NULL
};

struct lfb_req {
    uint8_t     opcode;
    uint32_t    length;
    uint32_t    offset;
    uint32_t    color;
    void *      data;
};

static inline void 
fast_memcpy_d(void *dst, const void *src, size_t nbyte)
{
    asm volatile("cld\n\t"
                 "rep ; movsd"
                 : "=D" (dst), "=S" (src)
                 : "c" (nbyte / 4), "D" (dst), "S" (src)
                 : "memory");
}

static void
fb_putc(struct lfb_state *state, int val, int fg_col, int bg_col)
{
    int x = state->position % state->textscreen_width * FONT_WIDTH;
    int y = state->position / state->textscreen_width * FONT_HEIGHT;

    if (val > 128) {
        val = 4;
    }
    uint32_t *fb = (uint32_t*)state->frame_buffer;
    uint32_t *fg = (uint32_t*)state->foreground;
    uint32_t *bg = (uint32_t*)state->background;

    uint8_t *c = number_font[val];

    uint32_t row[FONT_WIDTH];

    for (int j = 0 ; j < FONT_HEIGHT; j++) {
        int pos = (y + j) * state->width + x;
        for (int i = 0; i < FONT_WIDTH; i++) {
            if (c[j] & (1 << (8-i))) {
                row[i] = fg_col;
            } else {       
                row[i] = bg_col;
            }
        }
        
        fast_memcpy_d(&fg[pos], row, sizeof(row));

        for (int i = 0; i < FONT_WIDTH; i++) {
            if (row[i] == 0) {
                row[i] = bg[pos + i];
            }
        }

        fast_memcpy_d(&fb[pos], row, sizeof(row));
    }
}

static void
fb_redraw_background(struct lfb_state *state)
{
    uint32_t *bb = (uint32_t*)state->backbuffer;
    uint32_t *fg = (uint32_t*)state->foreground;
    uint32_t *bg = (uint32_t*)state->background;
   
    fast_memcpy_d(bb, fg, state->buffer_size);
 
    for (int i = 0; i < state->buffer_size / 4; i++) {
        if (!fg[i]) {
            bb[i] = bg[i];
        }
    }

    fast_memcpy_d(state->frame_buffer, bb, state->buffer_size);
}

static void 
fb_scroll(struct lfb_state *state)
{
    uint32_t *fg = (uint32_t*)state->foreground;

    int row_height = state->pitch * FONT_HEIGHT;

    fast_memcpy_d(fg, (void*)fg + row_height, state->buffer_size - row_height);

    memset((void*)fg + state->buffer_size - row_height, 0, row_height);

    fb_redraw_background(state);

    state->position = state->textscreen_width * (state->textscreen_height - 1);
}

#define ALPHA_BLEND(alpha, src, dst) (((src * ((alpha << 8) + alpha) + dst * ( (alpha ^ 0xFF) << 8) + (alpha ^ 0xFF) ) ) >> 16)

static void
shade_background(struct lfb_state *state, uint8_t alpha)
{
    uint8_t *bg = (uint8_t*)state->background;

    for (int i = 0; i < state->buffer_size; i += 4) {
        for (int j = 0; j < 4; j++) {
            bg[i+j] = ALPHA_BLEND(alpha, bg[i+j], 0x00);
        }
    }
}

static void
handle_lfb_request(struct lfb_state *state, struct lfb_req *req)
{
    switch (req->opcode) {
        case 0x00:
            memcpy(state->background + req->offset, req->data, req->length);
            break;
        case 0x01:
            shade_background(state, req->color);
            fb_redraw_background(state);
            break;
    }
}

static int
lfb_close(struct device *dev)
{
    return 0;
}

static int
lfb_ioctl(struct device *dev, uint64_t request, uintptr_t argp)
{
    struct lfb_state *state = (struct lfb_state*)dev->state;

    spinlock_lock(&state->lock);

    switch (request) {
    case TEXTSCREEN_CLEAR:
        state->position = 0;
        memcpy(state->frame_buffer, state->foreground, 4*state->height*state->width);
        break;
    case TEXTSCREEN_SETBG:
        state->background_color = state->color_palette[(uint8_t)argp];
        break;
    case TEXTSCREEN_SETFG:
        state->foreground_color = state->color_palette[(uint8_t)argp];
        break;
    case 0x200:
        handle_lfb_request(state, (struct lfb_req*)argp);
        break;
    }

    spinlock_unlock(&state->lock);
    return 0;
}

static int
lfb_isatty(struct device *dev)
{
    return 1;
}

static int
lfb_open(struct device *dev)
{
    return 0;
}

static int
lfb_write(struct device *dev, const char *buf, size_t nbyte, uint64_t pos)
{
    struct lfb_state *state = (struct lfb_state*)dev->state;

    spinlock_lock(&state->lock);

    for (int i = 0; i < nbyte; i++) {
       
        switch (buf[i]) {
            case '\n':
                state->position = (state->position + state->textscreen_width) - (state->position + state->textscreen_width) % state->textscreen_width;
                break;
            case '\b':
                state->position--;
                fb_putc(state, ' ', state->foreground_color, 0);
                break;
            default:
                fb_putc(state, buf[i], state->foreground_color, 0);
                state->position++;
                break;
                    
        } 

        if (state->position >= (state->textscreen_width * state->textscreen_height)) {
            fb_scroll(state);
        }
    }

    spinlock_unlock(&state->lock);

    return nbyte;
}

__attribute__((constructor))
void
_init_lfb()
{
    /* defined in sys/i686/kern/preinit.c */
    extern multiboot_info_t *multiboot_header;

    static struct lfb_state state;
    state.vbe = (vbe_info_t*)(0xC0000000 + multiboot_header->vbe_mode_info);
    state.pitch = state.vbe->pitch;
    state.depth = state.vbe->pitch / state.vbe->Xres;
    state.frame_buffer = (uint8_t*)state.vbe->physbase;
    state.textscreen_width = state.vbe->Xres / FONT_WIDTH;
    state.textscreen_height = state.vbe->Yres / FONT_HEIGHT;
    state.width = state.vbe->Xres;
    state.height = state.vbe->Yres;
    state.buffer_size = state.vbe->Xres * state.vbe->Yres * 4;
    state.position = 0;
    state.color_palette = default_color_palette;
    state.foreground = calloc(1, state.buffer_size);
    state.background = calloc(1, state.buffer_size);
    state.backbuffer = calloc(1, state.buffer_size);
    spinlock_unlock(&state.lock);
    lfb_device.state = &state;
    device_register(&lfb_device);
}


