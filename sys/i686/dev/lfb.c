#include <machine/multiboot.h>
#include <machine/vm.h>
#include <sys/device.h>
#include <sys/devices.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/timer.h>
#include <sys/types.h>
#include <sys/vm.h>

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
    int             textscreen_size;
    int             position;
    int             last_position;
    int             background_color;
    int             foreground_color;
    int *           color_palette;
    bool            enable_cursor;
    bool            show_cursor_next_tick;
};

static int default_color_palette[] = {0x0, 0x8c5760, 0x7b8c58, 0x8c6e43, 0x58698c, 0x7b5e7d, 0x66808c, 0x8c8b8b};

static int lfb_close(struct cdev *dev);
static int lfb_ioctl(struct cdev *dev, uint64_t request, uintptr_t argp);
static int lfb_mmap(struct cdev *dev, uintptr_t addr, size_t size, int prot, off_t offset);
static int lfb_open(struct cdev *dev);
static int lfb_write(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos);

struct cdev lfb_device = {
    .name       =   "lfb",
    .mode       =   0600,
    .majorno    =   DEV_MAJOR_CON,
    .minorno    =   1,
    .close      =   lfb_close,
    .ioctl      =   lfb_ioctl,
    .isatty     =   NULL,
    .mmap       =   lfb_mmap,
    .open       =   lfb_open,
    .read       =   NULL,
    .write      =   lfb_write,
    .state      =   NULL
};

struct lfb_req {
    uint8_t     opcode;
    uint32_t    length;
    uint32_t    offset;
    uint32_t    color;
    void *      data;
};

struct lfb_info {
    unsigned short  width;
    unsigned short  height;
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
fb_draw_cursor(struct lfb_state *state, int position, bool clear)
{
    uint32_t *fb = (uint32_t*)state->frame_buffer;
    uint32_t *fg = (uint32_t*)state->foreground;
    uint32_t *bg = (uint32_t*)state->background;

    int x = position % state->textscreen_width * FONT_WIDTH;
    int y = position / state->textscreen_width * FONT_HEIGHT;

    uint32_t row[FONT_WIDTH];

    for (int j = FONT_HEIGHT - 2 ; j < FONT_HEIGHT; j++) {
        int pos = (y + j) * state->width + x;
        for (int i = 0; i < FONT_WIDTH; i++) {

            if (!clear) {
                row[i] = state->foreground_color;
            } else {
                row[i] = fg[pos + i];

                if (row[i] == 0) {
                    row[i] = bg[pos + i];
                }
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

static void
clear_line(struct lfb_state *state, int n)
{
    while (state->position >= (state->textscreen_width * state->textscreen_height)) {
        state->position = state->position - state->textscreen_width;
    }
    int start_x;
    int end_x;

    switch (n) {
        case 0:
            start_x = state->position % state->textscreen_width * FONT_WIDTH;
            end_x = FONT_WIDTH * state->textscreen_width; //start_x + (state->width - (start_x % state->width));
            break;
        case 1:
            start_x = 0;
            end_x = state->position % state->textscreen_width * FONT_WIDTH;
            break;
        case 2:
            start_x = 0;
            end_x = FONT_WIDTH * state->textscreen_width;
            break;
    }

    int y = state->position / state->textscreen_width * FONT_HEIGHT;

    uint32_t *fg = (uint32_t*)state->foreground;
    uint32_t *fb = (uint32_t*)state->frame_buffer;
    uint32_t *bg = (uint32_t*)state->background;

    for (int i = 0; i < FONT_HEIGHT; i++) {
        int start_off = (y + i) * state->width + start_x;
        int end_off = (y + i) * state->width + end_x;
        int size = (end_off - start_off)*4;

        fast_memcpy_d(&fb[start_off], &bg[start_off], size);
        memset(&fg[start_off], 0, size);
    }
}

static int
lfb_close(struct cdev *dev)
{
    return 0;
}

static int
lfb_ioctl(struct cdev *dev, uint64_t request, uintptr_t argp)
{
    struct lfb_state *state = (struct lfb_state*)dev->state;

    spinlock_lock(&state->lock);

    switch (request) {
    case TXIOCLRSCR:
        state->position = 0;
        memset(state->foreground, 0, state->buffer_size);
        fast_memcpy_d(state->frame_buffer, state->background, state->buffer_size);
        break;
    case TXIOSETBG :
        state->background_color = state->color_palette[(uint8_t)argp];
        break;
    case TXIOSETFG :
        state->foreground_color = state->color_palette[(uint8_t)argp];
        break;
    case TXIOSETCUR: 
        state->position = ((struct curpos*)argp)->c_row * state->textscreen_width + ((struct curpos*)argp)->c_col;
        break; 
    case TXIOGETCUR:
        ((struct curpos*)argp)->c_row = state->position / state->textscreen_width;
        ((struct curpos*)argp)->c_col = state->position % state->textscreen_width;
        break;
    case FBIOBUFRQ:
        handle_lfb_request(state, (struct lfb_req*)argp);
        break;
    case TXIOERSLIN:       
        clear_line(state, (int)argp);
        break;
    case TXIOCURSON:
        state->enable_cursor = true;
        break;
    case TXIOCURSOFF:
        state->enable_cursor = false;
        fb_draw_cursor(state, state->last_position, true);
        break;
    case TXIORST:
        state->enable_cursor = false;
        state->position = 0;
        state->foreground_color = 0xFFFFFF;
        state->background_color = 0;
        break;
    case FBIOGETINFO:
        ((struct lfb_info*)argp)->width = state->width;
        ((struct lfb_info*)argp)->height = state->height;
        break;
    }

    spinlock_unlock(&state->lock);
    return 0;
}

static intptr_t
lfb_mmap(struct cdev *dev, uintptr_t addr, size_t size, int prot, off_t offset)
{
    /* defined in sys/i686/kern/preinit.c */
    extern multiboot_info_t *multiboot_header;

    struct lfb_state *state = (struct lfb_state*)dev->state;

    if (addr != 0) {
        return -(EINVAL);
    }

    if (size != state->buffer_size) {
        return -(EINVAL);
    }

    vbe_info_t *info = (vbe_info_t*)(KERNEL_VIRTUAL_BASE + multiboot_header->vbe_mode_info);

    extern struct vm_space *sched_curr_address_space;

    void *buf = vm_map_physical(sched_curr_address_space, NULL, info->physbase, state->buffer_size, prot);

    return (intptr_t)buf;
}

static int
lfb_open(struct cdev *dev)
{
    return 0;
}

static int
lfb_write(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos)
{
    struct lfb_state *state = (struct lfb_state*)dev->state;

    spinlock_lock(&state->lock);

    for (int i = 0; i < nbyte; i++) {
        if (state->position >= (state->textscreen_width * state->textscreen_height)) {
            fb_scroll(state);
        }
        switch (buf[i]) {
            case '\n':
                state->position += state->textscreen_width;
                break;
            case '\b':
                state->position--;
                fb_putc(state, ' ', state->foreground_color, state->background_color);
                break;
            case '\r':
                state->position -= state->position % state->textscreen_width;
                break;
            default:
                fb_putc(state, buf[i], state->foreground_color, state->background_color);
                state->position++;
                break;
                    
        } 
    }

    fb_draw_cursor(state, state->last_position, true);

    spinlock_unlock(&state->lock);

    return nbyte;
}

static void
lfb_tick(struct timer *timer, void *argp)
{
    struct lfb_state *state = (struct lfb_state*)argp;

    if (!state->enable_cursor || state->position >= state->textscreen_size) {
        timer_renew(timer, 1000);
        return;
    }

    if (state->position != state->last_position) {
        fb_draw_cursor(state, state->last_position, true);
    }

    fb_draw_cursor(state, state->position, state->show_cursor_next_tick);
    state->last_position = state->position;
    state->show_cursor_next_tick = !state->show_cursor_next_tick;

    timer_renew(timer, 500);
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
    state.textscreen_size = state.textscreen_width * state.textscreen_height;
    state.width = state.vbe->Xres;
    state.height = state.vbe->Yres;
    state.buffer_size = state.vbe->Xres * state.vbe->Yres * 4;
    state.position = 0;
    state.color_palette = default_color_palette;
    state.foreground = calloc(1, state.buffer_size);
    state.background = calloc(1, state.buffer_size);
    state.backbuffer = calloc(1, state.buffer_size);
    state.foreground_color = 0xFFFFFF;
    spinlock_unlock(&state.lock);
    lfb_device.state = &state;
    cdev_register(&lfb_device);

    timer_new(lfb_tick, 300, &state);
}


