/*
 * ioctl.h
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
#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H
#ifdef __cplusplus
extern "C" {
#endif

/* textscreen ioctls */
#define TXIOCLRSCR        0x0000
#define TXIOSETBG         0x0001
#define TXIOSETFG         0x0002
#define TXIOSETCUR        0x0003
#define TXIOGETCUR        0x0004
#define TXIOERSLIN        0x0005
#define TXIOCURSOFF       0x0006
#define TXIOCURSON        0x0007
#define TXIORST           0x0008
#define TXIODEFBG         0x0009
#define TXIODEFFG         0x000A
#define TXIOSETPAL        0x000B

#define FBIOBUFRQ         0x0200
#define FBIOGETINFO       0x0201

/* generic file descriptor ioctl's */

#define FIONREAD          0x80

struct curpos {
    unsigned short  c_row;
    unsigned short  c_col;
};

struct lfb_info {
    unsigned short    width;
    unsigned short    height;
};

struct winsize {
    unsigned short ws_row;    /* rows, in characters */
    unsigned short ws_col;    /* columns, in characters */
    unsigned short ws_xpixel; /* horizontal size, pixels */
    unsigned short ws_ypixel; /* vertical size, pixels */
};

struct palentry {
    unsigned char   p_index;
    unsigned int    p_col;
};

#ifndef __KERNEL__
int ioctl(int, unsigned long, void *);
#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif
#endif /* _SYS_IOCTL_H */
