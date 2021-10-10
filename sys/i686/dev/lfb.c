/*
 * lfb.c - Linear framebuffer 
 *
 * Exposes VBE framebuffer to userspace and also provides functionality to put
 * text to the framebuffer.
 *
 * Note to self: This file bas become a cluster fuck, plz fix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <machine/multiboot.h>
#include <machine/vm.h>
#include <sys/cdev.h>
#include <sys/device.h>
#include <sys/devno.h>
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
    uint8_t *       framebuffer;
    uint8_t *       foreground;
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

//static int default_color_palette[] = {0xFFFFFF, 0x151515, 0xAC4142, 0x90A959, 0xF4BF75, 0x6A9FB5, 0xAA759F, 0x00, 0x00, 0x00};
static int default_color_palette[] = {
  0x0000000, 0x1e92f2f, 0x20ed839, 0x3dddd13,
  0x43b48e3, 0x5f996e2, 0x623edda, 0x7ababab,
  0x8343434, 0x9e92f2f, 0xA0ed839, 0xBdddd13,
  0xC3b48e3, 0xDf996e2, 0xE23edda, 0xFf9f9f9,
  /* background */
  0xF9F9F9,
  /* foreground */
  0x102015
};

static int lfb_attach(struct driver *, struct device *);
static int lfb_ioctl(struct cdev *, uint64_t, uintptr_t);
static int lfb_mmap(struct cdev *, uintptr_t, size_t, int, off_t);
static int lfb_write(struct cdev *, const char *, size_t, uint64_t);

struct driver lfb_driver = {
    .attach     =   lfb_attach,
    .deattach   =   NULL,
    .probe      =   NULL
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

void *
fast_memset(void *ptr, uint32_t value, size_t nbyte)
{   
    int i;
    uint32_t *buf;

    buf = (uint32_t*)ptr;
    
    for (i = 0; i < nbyte / 4; i++) {
        buf[i] = value;
    }
    
    return ptr;
}   

static void
fb_putc(struct lfb_state *state, int val, int fg_col, int bg_col)
{
    int x;
    int y;
    int j;
    int i;
    int pos;

    uint32_t row[FONT_WIDTH];

    uint32_t *fb;
    uint32_t *fg;
    uint8_t *c;

    x = state->position % state->textscreen_width * FONT_WIDTH;
    y = state->position / state->textscreen_width * FONT_HEIGHT;

    if (val > 128) {
        val = 4;
    }

    fb = (uint32_t*)state->framebuffer;
    fg = (uint32_t*)state->foreground;
    c = &number_font[val*16];

    for (j = 0 ; j < FONT_HEIGHT; j++) {
        pos = (y + j) * state->width + x;

        for (i = 0; i < FONT_WIDTH; i++) {
            if (c[j] & (1 << i)) {
                row[i] = fg_col;
            } else {       
                row[i] = bg_col;
            }
        }
        
        fast_memcpy_d(&fg[pos], row, sizeof(row));
        fast_memcpy_d(&fb[pos], row, sizeof(row));
    }
}

static void
fb_draw_cursor(struct lfb_state *state, int position, bool clear)
{
    int x;
    int y;
    int j;
    int i;
    int pos;

    uint32_t *fb;
    uint32_t *fg;

    uint32_t row[FONT_WIDTH];

    fb = (uint32_t*)state->framebuffer;
    fg = (uint32_t*)state->foreground;

    x = position % state->textscreen_width * FONT_WIDTH;
    y = position / state->textscreen_width * FONT_HEIGHT;

    for (j = FONT_HEIGHT - 2 ; j < FONT_HEIGHT; j++) {
        pos = (y + j) * state->width + x;
        for (i = 0; i < FONT_WIDTH; i++) {
            if (!clear) {
                row[i] = state->foreground_color;
            } else {
                row[i] = fg[pos + i];
            }
        }
        fast_memcpy_d(&fb[pos], row, sizeof(row));
    }
}

static void 
fb_scroll(struct lfb_state *state)
{
    int row_height;
    uint32_t *fg;

    fg = (uint32_t*)state->foreground;
    row_height = state->pitch * FONT_HEIGHT;

    fast_memcpy_d(fg, (void*)fg + row_height, state->buffer_size - row_height);
    fast_memset((void*)fg + state->buffer_size - row_height, state->background_color, row_height);
    fast_memcpy_d(state->framebuffer, fg, state->buffer_size);

    state->position = state->textscreen_width * (state->textscreen_height - 1);
}

static void
lfb_tick(struct timer *timer, void *argp)
{
    struct lfb_state *state;

    state = (struct lfb_state*)argp;

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

static int
lfb_attach(struct driver *driver, struct device *dev)
{
    /* defined in sys/i686/kern/preinit.c */
    extern multiboot_info_t *multiboot_header;
    
    static struct lfb_state state;

    struct cdev *cdev;
    struct cdev_ops lfb_ops;

    lfb_ops = (struct cdev_ops) {
        .close  = NULL,
        .init   = NULL,
        .ioctl  = lfb_ioctl,
        .isatty = NULL,
        .mmap   = lfb_mmap,
        .open   = NULL,
        .read   = NULL,
        .write  = lfb_write
    };

    state.vbe = (vbe_info_t*)(0xC0000000 + multiboot_header->vbe_mode_info);
    state.pitch = state.vbe->pitch;
    state.depth = state.vbe->pitch / state.vbe->Xres;
    state.framebuffer = (uint8_t*)state.vbe->physbase;
    state.textscreen_width = state.vbe->Xres / FONT_WIDTH;
    state.textscreen_height = state.vbe->Yres / FONT_HEIGHT;
    state.textscreen_size = state.textscreen_width * state.textscreen_height;
    state.width = state.vbe->Xres;
    state.height = state.vbe->Yres;
    state.buffer_size = state.vbe->Xres * state.vbe->Yres * 4;
    state.position = 0; 
    state.color_palette = default_color_palette;
    state.foreground = calloc(1, state.buffer_size);
    state.foreground_color = default_color_palette[0x11];
    state.background_color = default_color_palette[0x10];

    fast_memset(state.foreground, state.background_color, state.buffer_size);
    fast_memset(state.framebuffer, state.background_color, state.buffer_size);
    
    spinlock_unlock(&state.lock);
    timer_new(lfb_tick, 300, &state);

    cdev = cdev_new("lfb", 0666, DEV_MAJOR_CON, 0, &lfb_ops, &state);

    if (cdev && cdev_register(cdev) == 0) {
        return 0;
    }

    return -1;
}

static int
lfb_ioctl(struct cdev *dev, uint64_t request, uintptr_t argp)
{
    struct lfb_state *state;

    state = (struct lfb_state*)dev->state;

    spinlock_lock(&state->lock);

    switch (request) {
    case TXIOCLRSCR:
        state->position = 0;
        memset(state->foreground, 0xFF, state->buffer_size);
        break;
    case TXIODEFBG:
        state->background_color = default_color_palette[0x10];
        break;
    case TXIODEFFG:
        state->foreground_color = default_color_palette[0x11];
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
        state->foreground_color = default_color_palette[0x11];
        state->background_color = default_color_palette[0x10];
        break;
    case TXIOSETPAL:
        default_color_palette[((struct palentry*)argp)->p_index] = ((struct palentry*)argp)->p_col;
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
    extern struct vm_space *sched_curr_address_space;

    void *buf;
    vbe_info_t *info;
    struct lfb_state *state;

    state = (struct lfb_state*)dev->state;

    if (addr != 0) {
        return -(EINVAL);
    }

    if (size != state->buffer_size) {
        return -(EINVAL);
    }

    info = (vbe_info_t*)(KERNEL_VIRTUAL_BASE + multiboot_header->vbe_mode_info);

    buf = vm_map_physical(sched_curr_address_space, NULL, info->physbase, state->buffer_size, prot);

    return (intptr_t)buf;
}

static int
lfb_write(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos)
{
    int i;
    struct lfb_state *state;

    state = dev->state;

    spinlock_lock(&state->lock);

    for (i = 0; i < nbyte; i++) {
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
