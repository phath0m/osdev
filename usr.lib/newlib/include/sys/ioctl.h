#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

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

#define FBIOBUFRQ         0x0200
#define FBIOGETINFO       0x0201

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

int ioctl(int fd, unsigned long request, void *arg);

#endif
