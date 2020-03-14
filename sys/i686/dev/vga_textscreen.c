/*
 * vga.c - VGA driver
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
#include <machine/portio.h>
#include <sys/device.h>
#include <sys/devno.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/string.h>
#include <sys/types.h>

#include "./vga_textscreen_font.h"

#define VGA_SET_MODE_320X200    0x100
#define VGA_DRAW                0x101
#define VGA_ENABLE_BUFFER       0x102
#define VGA_DRAW_BUFFER         0x103
#define VGA_SET_MODE_80X25_TEXT 0x104
#define VGA_SET_PALLETE_ENTRY   0x105

#define VGA_ADDR                0xC00A0000
#define TEXTSCREEN_ADDR         0xC00B8000
#define TEXTSCREEN_HEIGHT       25
#define TEXTSCREEN_WIDTH        80
#define TEXTSCREEN_BUFFER_SIZE  TEXTSCREEN_HEIGHT*TEXTSCREEN_WIDTH

#define VGA_SEQ_MAP_MASK_REG        0x02
#define VGA_SEQ_CHARSET_REG         0x03
#define VGA_SEQ_MEMORY_MODE_REG     0x04

#define VGA_GC_READ_MAP_SELECT_REG  0x04
#define VGA_GC_GRAPHICS_MODE_REG    0x05
#define VGA_GC_MISC_REG             0x06

#define VGA_AC_INDEX            0x3C0
#define VGA_AC_WRITE            0x3C0
#define VGA_AC_READ             0x3C1
#define VGA_MISC_WRITE          0x3C2
#define VGA_SEQ_INDEX           0x3C4
#define VGA_SEQ_DATA            0x3C5
#define VGA_DAC_READ_INDEX      0x3C7
#define VGA_DAC_WRITE_INDEX     0x3C8
#define VGA_DAC_DATA            0x3C9
#define VGA_MISC_READ           0x3CC
#define VGA_GC_INDEX            0x3CE
#define VGA_GC_DATA             0x3CF

/*          COLOR emulation     MONO emulation */
#define VGA_CRTC_INDEX          0x3D4       /* 0x3B4 */
#define VGA_CRTC_DATA           0x3D5       /* 0x3B5 */
#define VGA_INSTAT_READ         0x3DA

#define VGA_NUM_SEQ_REGS        5
#define VGA_NUM_CRTC_REGS       25
#define VGA_NUM_GC_REGS         9
#define VGA_NUM_AC_REGS         21
#define VGA_NUM_REGS            (1 + VGA_NUM_SEQ_REGS + VGA_NUM_CRTC_REGS + \
                                VGA_NUM_GC_REGS + VGA_NUM_AC_REGS)

uint8_t g_80x25_text[] = {
/* MISC */
    0x67,
/* SEQ */
    0x03, 0x00, 0x03, 0x00, 0x02,
/* CRTC */
    0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F,
    0x00, 0x47, 0x06, 0x07, 0x00, 0x00, 0x01, 0x40,
    0x9C, 0x8E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
    0xFF, 
/* GC */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
    0xFF, 
/* AC */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x0C, 0x00, 0x0F, 0x08, 0x00,
};

/* mode 13h */
uint8_t vga_320x200x256[] = {
    /* MISC */
    0x63,
    /* SEQ */
    0x03, 0x01, 0x0F, 0x00, 0x0E,
    /* CRTC */
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
    0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x9C, 0x0E, 0x8F, 0x28,   0x40, 0x96, 0xB9, 0xA3,
    0xFF,
    /* GC */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
    0xFF,
    /* AC */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x41, 0x00, 0x0F, 0x00,   0x00
};


struct vga_state {
    uint8_t     foreground_color;
    uint8_t     background_color;
    uint16_t    position;
    uint8_t     prev_regs[VGA_NUM_REGS];
    uint8_t *   video_buffer;
};

struct vga_state state;

static int vga_close(struct cdev *dev);
static int vga_init(struct cdev *dev);
static int vga_ioctl(struct cdev *dev, uint64_t request, uintptr_t argp);
static int vga_open(struct cdev *dev);
static int vga_write(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos);

struct cdev vga_device = {
    .name       =   "vga",
    .mode       =   0600,
    .majorno    =   DEV_MAJOR_CON,
    .minorno    =   0,
    .close      =   vga_close,
    .init       =   vga_init,
    .ioctl      =   vga_ioctl,
    .open       =   vga_open,
    .isatty     =   NULL,
    .read       =   NULL,
    .write      =   vga_write,
    .state      =   &state
};

struct draw_req {
    uint8_t     opcode;
    uint8_t     col;
    uint8_t     transparency_index;
    uint16_t    x;
    uint16_t    y;
    uint16_t    width;
    uint16_t    height;
    uint8_t *   data;
};

struct pallete_entry {
    uint16_t    index;
    uint8_t     red;
    uint8_t     green;
    uint8_t     blue;
};

static void
vga_set_regs(uint8_t *regs)
{   
    io_write_byte(VGA_MISC_WRITE, *regs);
    regs++;

    /* write SEQUENCER regs */
    for (int i = 0; i < VGA_NUM_SEQ_REGS; i++) {
      io_write_byte(VGA_SEQ_INDEX, i);
      io_write_byte(VGA_SEQ_DATA, *regs);
      regs++;
    }

    /* unlock CRTC registers */
    io_write_byte(VGA_SEQ_INDEX, 0x03);
    io_write_byte(VGA_SEQ_DATA, io_read_byte(VGA_CRTC_DATA) | 0x80);
    io_write_byte(VGA_SEQ_INDEX, 0x11);
    io_write_byte(VGA_SEQ_DATA, io_read_byte(VGA_CRTC_DATA) & ~0x80);

    /* make sure they remain unlocked */
    regs[0x03] |= 0x80;
    regs[0x11] &= ~0x80;

    /* write CRTC regs */
    for (int i = 0; i < VGA_NUM_CRTC_REGS; i++) {
        io_write_byte(VGA_CRTC_INDEX, i);
        io_write_byte(VGA_CRTC_DATA, *regs);
        regs++;
    }

    /* write GRAPHICS CONTROLLER regs */
    for (int i = 0; i < VGA_NUM_GC_REGS; i++) {
        io_write_byte(VGA_GC_INDEX, i);
        io_write_byte(VGA_GC_DATA, *regs);
        regs++;
    }

    /* write ATTRIBUTE CONTROLLER regs */
    for (int i = 0; i < VGA_NUM_AC_REGS; i++) {
        io_read_byte(VGA_INSTAT_READ);
        io_write_byte(VGA_AC_INDEX, i);
        io_write_byte(VGA_AC_WRITE, *regs);
        i++;
    }

    /* lock 16-color palette and unblank display */
    io_read_byte(VGA_INSTAT_READ);
    io_write_byte(VGA_AC_INDEX, 0x20);
}

static inline void
vga_write_reg(uint16_t iport, uint8_t reg, uint8_t val)
{
   io_write_byte(iport, reg);
   io_write_byte(iport + 1, val);
}

static inline uint8_t
vga_read_reg(uint16_t iport, uint8_t reg)
{
   io_write_byte(iport, reg);
   return io_read_byte(iport + 1);
}

static void
vga_write_font(uint8_t *buf, int font_height)
{
   vga_write_reg(VGA_SEQ_INDEX, VGA_SEQ_MAP_MASK_REG, 0x04);
   vga_write_reg(VGA_SEQ_INDEX, VGA_SEQ_CHARSET_REG, 0x00);

   uint8_t mem_mode = vga_read_reg(VGA_SEQ_INDEX, VGA_SEQ_MEMORY_MODE_REG);

   vga_write_reg(VGA_SEQ_INDEX, VGA_SEQ_MEMORY_MODE_REG, 0x06);
   vga_write_reg(VGA_GC_INDEX, VGA_GC_READ_MAP_SELECT_REG, 0x02);

   uint8_t graphics_mode = vga_read_reg(VGA_GC_INDEX, VGA_GC_GRAPHICS_MODE_REG);

   vga_write_reg(VGA_GC_INDEX, VGA_GC_GRAPHICS_MODE_REG, 0x00);
   vga_write_reg(VGA_GC_INDEX, VGA_GC_MISC_REG, 0x0C);

    for(int i = 0; i < 256; i++) {
        memcpy((void*)(TEXTSCREEN_ADDR + i*32), buf, font_height);
        buf += font_height;
    }

   vga_write_reg(VGA_SEQ_INDEX, VGA_SEQ_MAP_MASK_REG, 0x03);
   vga_write_reg(VGA_SEQ_INDEX, VGA_SEQ_MEMORY_MODE_REG, mem_mode);
   vga_write_reg(VGA_GC_INDEX, VGA_GC_READ_MAP_SELECT_REG, 0x00);
   vga_write_reg(VGA_GC_INDEX, VGA_GC_GRAPHICS_MODE_REG, graphics_mode);
   vga_write_reg(VGA_GC_INDEX, VGA_GC_MISC_REG, 0x0C);
}

static void
vga_set_pallete_color(struct pallete_entry *entry)
{
    io_write_byte(0x03c8, entry->index);
    io_write_byte(0x03c9, entry->red);
    io_write_byte(0x03c9, entry->green);
    io_write_byte(0x03c9, entry->blue);
}

static void
vga_clear(struct vga_state *state, uint8_t col)
{
    memset(state->video_buffer, col, 320*200);
}

static void
vga_fill(struct vga_state *state, struct draw_req *req)
{
    for (int x = 0; x < req->width; x++)
    for (int y = 0; y < req->height; y++) {
        int pos = 320 * (y + req->y) + (x + req->x);
        state->video_buffer[pos] = req->col;
    }
}

static void
vga_set_pixels(struct vga_state *state, struct draw_req *req)
{
    memcpy((uint8_t*)VGA_ADDR, req->data, 320*200);
}

static void
vga_draw(struct vga_state *state, struct draw_req *req)
{
    switch (req->opcode) {
        case 0:
            vga_fill(state, req);
            break;
        case 1:
            vga_set_pixels(state, req);
            break;
        case 2:
            vga_clear(state, req->col);
            break;
        default:
            break;
    }
}

static void
textscreen_update_cursor(uint16_t pos)
{
    io_write_byte(0x3D4, 0x0F);
    io_write_byte(0x3D5, (uint8_t) (pos & 0xFF));
    io_write_byte(0x3D4, 0x0E);
    io_write_byte(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

static void
textscreen_enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
    io_write_byte(0x3D4, 0x0A);
    io_write_byte(0x3D5, (io_read_byte(0x3D5) & 0xC0) | cursor_start);
 
    io_write_byte(0x3D4, 0x0B);
    io_write_byte(0x3D5, (io_read_byte(0x3D5) & 0xE0) | cursor_end);
}

static void
textscreen_clear(uint8_t attr)
{
    uint16_t *vga_buffer = (uint16_t*)TEXTSCREEN_ADDR;
    uint16_t cell_value = ' ' | (attr << 8);

    for (int y = 0; y < TEXTSCREEN_HEIGHT; y++) {
        for (int x = 0; x < TEXTSCREEN_WIDTH; x++) {
            int pos = y * TEXTSCREEN_WIDTH + x;
            vga_buffer[pos] = cell_value;
        }
    }

}

static inline void
textscreen_scroll(uint8_t attr)
{
    uint16_t *vga_buffer = (uint16_t*)TEXTSCREEN_ADDR;

    for (int y = 1; y < TEXTSCREEN_HEIGHT; y++) {
        for (int x = 0; x < TEXTSCREEN_WIDTH; x++) {
            int pos = y * TEXTSCREEN_WIDTH + x;
            vga_buffer[pos - TEXTSCREEN_WIDTH] = vga_buffer[pos];
        }
    }

    for (int x = 0; x < TEXTSCREEN_WIDTH; x++) {
        int y = TEXTSCREEN_WIDTH * (TEXTSCREEN_HEIGHT - 1);

        vga_buffer[y + x] = ' ' | (attr << 8);
    }
}

static int
vga_close(struct cdev *dev)
{
    return 0;
}

static int
vga_init(struct cdev *dev)
{
    state.position = 0;
    state.foreground_color = 15;
    state.background_color = 0;
    state.video_buffer = (uint8_t*)0xC00A0000;

    return 0;
}

static int
vga_ioctl(struct cdev *dev, uint64_t request, uintptr_t argp)
{
    struct vga_state *statep = (struct vga_state*)dev->state;

    switch (request) {
    case TXIOCLRSCR:
        textscreen_clear((statep->background_color << 4) | statep->foreground_color);
        break;
    case TXIOSETBG:
        statep->background_color = (uint8_t)argp;
        break;
    case TXIOSETFG:
        statep->foreground_color = (uint8_t)argp;
        break;
    case VGA_SET_MODE_320X200:
        vga_set_regs(vga_320x200x256);
        vga_clear(statep, 0);        
        break;
    case VGA_SET_MODE_80X25_TEXT:
        vga_set_regs(g_80x25_text);
        vga_write_font(vga_8x16_bin, 16);
        textscreen_enable_cursor(13, 15);
        break;
    case VGA_SET_PALLETE_ENTRY:
        vga_set_pallete_color((struct pallete_entry*)argp);
        break;
    case VGA_DRAW:
        vga_draw(statep, (struct draw_req*)argp);
        break;
    }

    return 0;
}

static int
vga_open(struct cdev *dev)
{
    return 0;
}

static int
vga_write(struct cdev *dev, const char *buf, size_t nbyte, uint64_t pos)
{
    struct vga_state *statep = (struct vga_state*)dev->state;

    uint16_t *vga_buffer = (uint16_t*)TEXTSCREEN_ADDR;

    for (int i = 0; i < nbyte; i++) {
        uint8_t attr = (statep->background_color << 4) | statep->foreground_color;

        char ch = buf[i];

        switch (ch) {
        case '\n':
            statep->position = (statep->position + 80) - (statep->position + 80) % 80;
            break;
        case '\r':
            statep->position -= statep->position % 80;
            break;
        case '\b':
            vga_buffer[--statep->position] = ' ' | (attr < 8);
            break;
        default:
            vga_buffer[statep->position++] = ch | (attr << 8);
            break;
        }
        
        if (statep->position >= TEXTSCREEN_BUFFER_SIZE) {
            textscreen_scroll(attr);
            statep->position = TEXTSCREEN_BUFFER_SIZE - TEXTSCREEN_WIDTH;
        }
    }

    textscreen_update_cursor(statep->position);

    return nbyte;
}
